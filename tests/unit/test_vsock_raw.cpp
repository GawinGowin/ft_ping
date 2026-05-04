#include <gtest/gtest.h>
#include <netinet/in.h>

extern "C" {
#include "vsock/vsock.h"
}

class VsockRawOpsTest : public ::testing::Test {
protected:
  t_ipheader_ctx make_ctx(size_t datalen) {
    t_ipheader_ctx ctx = {};
    ctx.datalen = datalen;
    ctx.src.s_addr = htonl(0x7f000001); // 127.0.0.1
    ctx.dst.s_addr = htonl(0x7f000001);
    return ctx;
  }
};

TEST_F(VsockRawOpsTest, VtablePointersAreSet) {
  EXPECT_NE(Ping_socket_raw_ops.build_ipicmp, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.extract_icmp, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.extract_ttl, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.packet_size, nullptr);
  EXPECT_NE(Ping_socket_raw_ops.extra_configure, nullptr);
}

// extract_ttl_raw: 受信パケット先頭の IP ヘッダーから TTL を取り出す
TEST_F(VsockRawOpsTest, ExtractTtlReadsIpHeader) {
  uint8_t buf[sizeof(struct iphdr) + sizeof(struct icmphdr)] = {};
  struct iphdr *ip = (struct iphdr *)buf;
  ip->ihl = 5;
  ip->ttl = 64;

  EXPECT_EQ(Ping_socket_raw_ops.extract_ttl(buf, nullptr), 64);
}

TEST_F(VsockRawOpsTest, ExtractTtlReturnsZeroForNullPacket) {
  EXPECT_EQ(Ping_socket_raw_ops.extract_ttl(nullptr, nullptr), 0);
}

TEST_F(VsockRawOpsTest, PacketSizeIncludesIpHeader) {
  size_t datalen = 56;
  size_t size = Ping_socket_raw_ops.packet_size(datalen);
  EXPECT_EQ(size, sizeof(t_ip_icmp) + datalen);
}

// build_ipicmp_raw は IP ヘッダーのみを書く（ICMP は呼び出し元が書く）
TEST_F(VsockRawOpsTest, BuildIpheaderSetsIpFields) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_raw_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_ipheader_ctx ctx = make_ctx(datalen);
  int ret = Ping_socket_raw_ops.build_ipicmp(buf, &ctx);
  EXPECT_EQ(ret, 0);

  t_ip_icmp *pkt = (t_ip_icmp *)buf;
  EXPECT_EQ(pkt->ip.version, 4);
  EXPECT_EQ(pkt->ip.protocol, IPPROTO_ICMP);
  EXPECT_EQ(ntohs(pkt->ip.tot_len), (uint16_t)(sizeof(t_ip_icmp) + datalen));
  EXPECT_EQ(pkt->ip.saddr, ctx.src.s_addr);
  EXPECT_EQ(pkt->ip.daddr, ctx.dst.s_addr);
  EXPECT_NE(pkt->ip.check, 0); // IP チェックサムは計算済み
  free(buf);
}

TEST_F(VsockRawOpsTest, BuildIpheaderFailsOnNull) {
  t_ipheader_ctx ctx = make_ctx(56);
  EXPECT_EQ(Ping_socket_raw_ops.build_ipicmp(NULL, &ctx), -1);
  EXPECT_EQ(Ping_socket_raw_ops.build_ipicmp((void *)1, NULL), -1);
}

// extract_icmp_raw: IP ヘッダー（ihl*4 バイト）をスキップして ICMP へのポインタを返す
TEST_F(VsockRawOpsTest, ExtractIcmpSkipsIpHeader) {
  size_t datalen = 56;
  size_t pkt_size = Ping_socket_raw_ops.packet_size(datalen);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  // IP ヘッダーだけ手動設定（extract_icmp が ihl を読むため）
  struct iphdr *ip = (struct iphdr *)buf;
  ip->ihl = 5; // 20 bytes
  // ICMP 領域に識別用の値を書く
  struct icmphdr *icmp_expected = (struct icmphdr *)((char *)buf + sizeof(struct iphdr));
  icmp_expected->type = ICMP_ECHOREPLY;

  int icmp_len = 0;
  struct icmphdr *icmp = Ping_socket_raw_ops.extract_icmp(buf, pkt_size, &icmp_len);
  ASSERT_NE(icmp, nullptr);
  EXPECT_EQ(icmp->type, ICMP_ECHOREPLY);
  EXPECT_EQ(icmp_len, (int)(pkt_size - sizeof(struct iphdr)));
  free(buf);
}

TEST_F(VsockRawOpsTest, ExtractIcmpReturnsNullForTooShortPacket) {
  size_t pkt_size = sizeof(struct iphdr) + 4; // ICMP 部分が 8 バイト未満
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  struct iphdr *ip = (struct iphdr *)buf;
  ip->ihl = 5;

  int icmp_len = 0;
  struct icmphdr *icmp = Ping_socket_raw_ops.extract_icmp(buf, pkt_size, &icmp_len);
  EXPECT_EQ(icmp, nullptr);
  free(buf);
}
