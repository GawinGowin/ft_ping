#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

extern "C" {
#include "ping_icmp.h"
#include "shared/shared_net.h"
}

// ---------------------------------------------------------------------------
// inet_checksum
// ---------------------------------------------------------------------------

TEST(ChecksumTest, AllZeroesIsMaxValue) {
  uint8_t data[4] = {0, 0, 0, 0};
  uint16_t cs = inet_checksum(data, sizeof(data));
  EXPECT_EQ(cs, 0xffff);
}

TEST(ChecksumTest, ApplyingTwiceYieldsZero) {
  // RFC 1071: checksum of data + its own checksum == 0
  uint8_t data[8] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x00};
  uint16_t cs = inet_checksum(data, sizeof(data));
  // embed checksum and re-verify
  uint16_t *cs_field = (uint16_t *)(data + 2);
  *cs_field = cs;
  uint16_t verify = inet_checksum(data, sizeof(data));
  EXPECT_EQ(verify, 0);
}

TEST(ChecksumTest, OddLengthHandledCorrectly) {
  uint8_t data[3] = {0x00, 0x01, 0x02};
  // should not crash and must return consistent result
  uint16_t cs1 = inet_checksum(data, sizeof(data));
  uint16_t cs2 = inet_checksum(data, sizeof(data));
  EXPECT_EQ(cs1, cs2);
}

TEST(ChecksumTest, SingleByteAllOnes) {
  uint8_t data[1] = {0xff};
  uint16_t cs = inet_checksum(data, 1);
  EXPECT_EQ(cs, (uint16_t)~0x00ff);
}

TEST(ChecksumTest, KnownEchoHeader) {
  // type=8 code=0 checksum=0 id=1 seq=1  (ping echo request for id=1 seq=1)
  uint8_t hdr[8] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01};
  uint16_t cs = inet_checksum(hdr, sizeof(hdr));
  // embed and re-check
  uint16_t *cs_field = (uint16_t *)(hdr + 2);
  *cs_field = cs;
  EXPECT_EQ(inet_checksum(hdr, sizeof(hdr)), 0);
}

TEST(ChecksumTest, CarryFoldedCorrectly) {
  // two 0xffff words -> sum = 0x1fffe -> folded = 0xffff -> ~0xffff = 0
  uint8_t data[4] = {0xff, 0xff, 0xff, 0xff};
  uint16_t cs = inet_checksum(data, sizeof(data));
  EXPECT_EQ(cs, 0);
}

// ---------------------------------------------------------------------------
// ping_icmp_build_echo
// ---------------------------------------------------------------------------

class BuildEchoTest : public ::testing::Test {
protected:
  static constexpr size_t kDatalen = 56;

  void SetUp() override {
    ts_.tv_sec = 1234567890;
    ts_.tv_usec = 123456;
    buf_size_ = sizeof(struct icmphdr) + kDatalen;
    buf_ = new uint8_t[buf_size_]();
    icmp_ = (struct icmphdr *)buf_;
    payload_ = buf_ + sizeof(struct icmphdr);
  }

  void TearDown() override { delete[] buf_; }

  void build(uint16_t seq = 0, size_t datalen = kDatalen) {
    ping_icmp_build_echo(icmp_, payload_, seq, datalen, &ts_);
  }

  struct timeval ts_;
  size_t buf_size_;
  uint8_t *buf_;
  struct icmphdr *icmp_;
  unsigned char *payload_;
};

TEST_F(BuildEchoTest, TypeIsEcho) {
  build();
  EXPECT_EQ(icmp_->type, ICMP_ECHO);
}

TEST_F(BuildEchoTest, CodeIsZero) {
  build();
  EXPECT_EQ(icmp_->code, 0);
}

TEST_F(BuildEchoTest, ChecksumIsNonZero) {
  build();
  EXPECT_NE(icmp_->checksum, 0);
}

TEST_F(BuildEchoTest, ChecksumVerifies) {
  build();
  // save checksum, zero it, recompute
  uint16_t embedded = icmp_->checksum;
  icmp_->checksum = 0;
  uint16_t recomputed = inet_checksum(buf_, buf_size_);
  EXPECT_EQ(embedded, recomputed);
}

TEST_F(BuildEchoTest, IdMatchesPid) {
  build();
  EXPECT_EQ(ntohs(icmp_->un.echo.id), (uint16_t)getpid());
}

TEST_F(BuildEchoTest, SequenceIsSeqPlusOne) {
  build(/*seq=*/5);
  EXPECT_EQ(ntohs(icmp_->un.echo.sequence), 6);
}

TEST_F(BuildEchoTest, SequenceWrapsAt16Bit) {
  build(/*seq=*/0xffff);
  EXPECT_EQ(ntohs(icmp_->un.echo.sequence), 0);
}

TEST_F(BuildEchoTest, TimestampEmbeddedInPayload) {
  build();
  struct timeval recovered;
  memcpy(&recovered, payload_, sizeof(recovered));
  EXPECT_EQ(recovered.tv_sec, ts_.tv_sec);
  EXPECT_EQ(recovered.tv_usec, ts_.tv_usec);
}

TEST_F(BuildEchoTest, PayloadFilledWithPattern) {
  // datalen smaller than timeval so no timestamp overwrite
  constexpr size_t small = 4;
  uint8_t small_buf[sizeof(struct icmphdr) + small] = {};
  struct icmphdr *h = (struct icmphdr *)small_buf;
  unsigned char *p = small_buf + sizeof(struct icmphdr);
  ping_icmp_build_echo(h, p, 0, small, &ts_);
  for (size_t i = 0; i < small; i++)
    EXPECT_EQ(p[i], (unsigned char)(i % 255)) << "at index " << i;
}

TEST_F(BuildEchoTest, DifferentSeqYieldsDifferentChecksum) {
  build(/*seq=*/1);
  uint16_t cs1 = icmp_->checksum;
  memset(buf_, 0, buf_size_);
  build(/*seq=*/2);
  uint16_t cs2 = icmp_->checksum;
  EXPECT_NE(cs1, cs2);
}

TEST_F(BuildEchoTest, MinimalDatlenOneByte) {
  constexpr size_t tiny = 1;
  uint8_t b[sizeof(struct icmphdr) + tiny] = {};
  struct icmphdr *h = (struct icmphdr *)b;
  unsigned char *p = b + sizeof(struct icmphdr);
  ping_icmp_build_echo(h, p, 0, tiny, &ts_);
  EXPECT_EQ(h->type, ICMP_ECHO);
  // checksum verifies
  uint16_t cs = h->checksum;
  h->checksum = 0;
  EXPECT_EQ(cs, inet_checksum(b, sizeof(b)));
}
