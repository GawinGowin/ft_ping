#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/time.h>

extern "C" {
#include "vsock.h"
}

class VsockRawOpsTest : public ::testing::Test {
protected:
  t_build_ctx make_ctx(size_t datalen) {
    t_build_ctx ctx = {};
    ctx.seq = 0;
    ctx.datalen = datalen;
    ctx.ts = &ts_;
    gettimeofday(&ts_, NULL);
    ctx.src.s_addr = htonl(0x7f000001); // 127.0.0.1
    ctx.dst.s_addr = htonl(0x7f000001);
    return ctx;
  }

  struct timeval ts_;
};

TEST_F(VsockRawOpsTest, VtablePointersAreSet) {
  EXPECT_NE(Ping_socket_raw_ops.build_packet, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.extract_icmp, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.packet_size, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.extra_configure, nullptr);
}

TEST_F(VsockRawOpsTest, PacketSizeIncludesIpHeader) {
  size_t datalen = 56;
  size_t size = Ping_socket_raw_ops.packet_size(datalen);
  EXPECT_EQ(size, sizeof(t_ip_icmp) + datalen);
}

TEST_F(VsockRawOpsTest, BuildPacketSucceeds) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_raw_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_build_ctx ctx = make_ctx(datalen);
  int ret = Ping_socket_raw_ops.build_packet(buf, &ctx);
  EXPECT_EQ(ret, 0);

  t_ip_icmp *pkt = (t_ip_icmp *)buf;
  EXPECT_EQ(pkt->icmp.type, ICMP_ECHO);
  EXPECT_EQ(pkt->icmp.code, 0);
  EXPECT_EQ(pkt->ip.protocol, IPPROTO_ICMP);
  EXPECT_EQ(ntohs(pkt->ip.tot_len), (uint16_t)(sizeof(t_ip_icmp) + datalen));
  free(buf);
}

TEST_F(VsockRawOpsTest, BuildPacketFailsOnNull) {
  t_build_ctx ctx = make_ctx(56);
  EXPECT_EQ(Ping_socket_raw_ops.build_packet(NULL, &ctx), -1);
  EXPECT_EQ(Ping_socket_raw_ops.build_packet((void *)1, NULL), -1);
}

TEST_F(VsockRawOpsTest, ExtractIcmpFromValidPacket) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_raw_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_build_ctx ctx = make_ctx(datalen);
  Ping_socket_raw_ops.build_packet(buf, &ctx);

  int icmp_len = 0;
  struct icmphdr *icmp = Ping_socket_raw_ops.extract_icmp(buf, pkt_size, &icmp_len);
  ASSERT_NE(icmp, nullptr);
  EXPECT_EQ(icmp->type, ICMP_ECHO);
  EXPECT_GT(icmp_len, 0);
  free(buf);
}

TEST_F(VsockRawOpsTest, ChecksumIsNonZeroAfterBuild) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_raw_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_build_ctx ctx = make_ctx(datalen);
  Ping_socket_raw_ops.build_packet(buf, &ctx);

  t_ip_icmp *pkt = (t_ip_icmp *)buf;
  EXPECT_NE(pkt->icmp.checksum, 0);
  free(buf);
}
