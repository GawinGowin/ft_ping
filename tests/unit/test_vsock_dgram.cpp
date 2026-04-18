#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/time.h>

extern "C" {
#include "vsock.h"
}

class VsockDgramOpsTest : public ::testing::Test {
protected:
  t_build_ctx make_ctx(size_t datalen) {
    t_build_ctx ctx = {};
    ctx.seq = 0;
    ctx.datalen = datalen;
    ctx.ts = &ts_;
    gettimeofday(&ts_, NULL);
    ctx.src.s_addr = htonl(0x7f000001);
    ctx.dst.s_addr = htonl(0x7f000001);
    return ctx;
  }

  struct timeval ts_;
};

TEST_F(VsockDgramOpsTest, VtablePointersAreSet) {
  EXPECT_NE(Ping_socket_dgram_ops.build_packet, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, nullptr);
}

// DGRAM は IP ヘッダを自分で作らないので RAW より小さい
TEST_F(VsockDgramOpsTest, PacketSizeSmallerThanRaw) {
  size_t datalen = 56;
  size_t dgram_size = Ping_socket_dgram_ops.packet_size(datalen);
  size_t raw_size = Ping_socket_raw_ops.packet_size(datalen);
  EXPECT_EQ(dgram_size, sizeof(struct icmphdr) + datalen);
  EXPECT_LT(dgram_size, raw_size);
}

TEST_F(VsockDgramOpsTest, BuildPacketSucceeds) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_dgram_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_build_ctx ctx = make_ctx(datalen);
  int ret = Ping_socket_dgram_ops.build_packet(buf, &ctx);
  EXPECT_EQ(ret, 0);

  struct icmphdr *icmp = (struct icmphdr *)buf;
  EXPECT_EQ(icmp->type, ICMP_ECHO);
  EXPECT_EQ(icmp->code, 0);
  free(buf);
}

TEST_F(VsockDgramOpsTest, BuildPacketFailsOnNull) {
  t_build_ctx ctx = make_ctx(56);
  EXPECT_EQ(Ping_socket_dgram_ops.build_packet(NULL, &ctx), -1);
  EXPECT_EQ(Ping_socket_dgram_ops.build_packet((void *)1, NULL), -1);
}

TEST_F(VsockDgramOpsTest, ExtractIcmpReturnsPacketStart) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_dgram_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_build_ctx ctx = make_ctx(datalen);
  Ping_socket_dgram_ops.build_packet(buf, &ctx);

  int icmp_len = 0;
  struct icmphdr *icmp = Ping_socket_dgram_ops.extract_icmp(buf, pkt_size, &icmp_len);
  // DGRAM は IP ヘッダがないのでパケット先頭が ICMP ヘッダ
  EXPECT_EQ((void *)icmp, buf);
  EXPECT_EQ((size_t)icmp_len, pkt_size);
  free(buf);
}

// DGRAM の extra_configure は何もせず 0 を返す
TEST_F(VsockDgramOpsTest, ExtraConfigureIsNoop) {
  int ret = Ping_socket_dgram_ops.extra_configure(0);
  EXPECT_EQ(ret, 0);
}

// RAW と DGRAM で関数ポインタが異なること
TEST_F(VsockDgramOpsTest, VtableDiffersFromRaw) {
  EXPECT_NE(Ping_socket_dgram_ops.build_packet, Ping_socket_raw_ops.build_packet);
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp, Ping_socket_raw_ops.extract_icmp);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size, Ping_socket_raw_ops.packet_size);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, Ping_socket_raw_ops.extra_configure);
}
