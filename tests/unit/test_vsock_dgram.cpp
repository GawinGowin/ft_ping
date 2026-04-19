#include <gtest/gtest.h>
#include <netinet/in.h>

extern "C" {
#include "vsock/vsock.h"
}

TEST(VsockDgramOpsTest, VtablePointersAreSet) {
  EXPECT_NE(Ping_socket_dgram_ops.build_ipheader, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, nullptr);
}

// DGRAM は IP ヘッダを自分で作らないので RAW より小さい
TEST(VsockDgramOpsTest, PacketSizeSmallerThanRaw) {
  size_t datalen = 56;
  size_t dgram_size = Ping_socket_dgram_ops.packet_size(datalen);
  size_t raw_size   = Ping_socket_raw_ops.packet_size(datalen);
  EXPECT_EQ(dgram_size, sizeof(struct icmphdr) + datalen);
  EXPECT_LT(dgram_size, raw_size);
}

// DGRAM は IP ヘッダーを書かない no-op
TEST(VsockDgramOpsTest, BuildIpheaderIsNoop) {
  size_t pkt_size = Ping_socket_dgram_ops.packet_size(56);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);
  memset(buf, 0xAB, pkt_size); // 既知パターンで埋める

  t_ipheader_ctx ctx = {};
  ctx.datalen = 56;
  int ret = Ping_socket_dgram_ops.build_ipheader(buf, &ctx);
  EXPECT_EQ(ret, 0);

  // パケット内容が変更されていないこと（no-op の確認）
  unsigned char *p = (unsigned char *)buf;
  for (size_t i = 0; i < pkt_size; i++) {
    EXPECT_EQ(p[i], 0xAB) << "byte " << i << " was modified";
  }
  free(buf);
}

// extract_icmp_dgram: パケット先頭が直接 ICMP ヘッダー
TEST(VsockDgramOpsTest, ExtractIcmpReturnsPacketStart) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_dgram_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  int icmp_len = 0;
  struct icmphdr *icmp = Ping_socket_dgram_ops.extract_icmp(buf, pkt_size, &icmp_len);
  EXPECT_EQ((void *)icmp, buf); // IP ヘッダーがないのでパケット先頭 = ICMP ヘッダー
  EXPECT_EQ((size_t)icmp_len, pkt_size);
  free(buf);
}

// DGRAM の extra_configure は何もせず 0 を返す
TEST(VsockDgramOpsTest, ExtraConfigureIsNoop) {
  int ret = Ping_socket_dgram_ops.extra_configure(0);
  EXPECT_EQ(ret, 0);
}

// RAW と DGRAM で関数ポインタが異なること
TEST(VsockDgramOpsTest, VtableDiffersFromRaw) {
  EXPECT_NE(Ping_socket_dgram_ops.build_ipheader, Ping_socket_raw_ops.build_ipheader);
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp,   Ping_socket_raw_ops.extract_icmp);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size,    Ping_socket_raw_ops.packet_size);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, Ping_socket_raw_ops.extra_configure);
}
