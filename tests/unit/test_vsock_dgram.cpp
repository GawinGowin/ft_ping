#include <gtest/gtest.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

extern "C" {
#include "vsock/vsock.h"
}

TEST(VsockDgramOpsTest, VtablePointersAreSet) {
  EXPECT_NE(Ping_socket_dgram_ops.build_ipheader, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extract_ttl, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size, nullptr);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, nullptr);
}

// DGRAM は IP ヘッダを自分で作らないので RAW より小さい
TEST(VsockDgramOpsTest, PacketSizeSmallerThanRaw) {
  size_t datalen = 56;
  size_t dgram_size = Ping_socket_dgram_ops.packet_size(datalen);
  size_t raw_size = Ping_socket_raw_ops.packet_size(datalen);
  EXPECT_EQ(dgram_size, sizeof(struct icmphdr) + datalen);
  EXPECT_LT(dgram_size, raw_size);
}

// DGRAM は IP ヘッダーなしで ICMP ヘッダーを直接構築する
TEST(VsockDgramOpsTest, BuildIpheaderBuildsIcmpHeader) {
  size_t pkt_size = Ping_socket_dgram_ops.packet_size(56);
  void *buf = calloc(1, pkt_size);
  ASSERT_NE(buf, nullptr);

  t_ipheader_ctx ctx = {};
  ctx.datalen = 56;
  ctx.seq = 1;
  int ret = Ping_socket_dgram_ops.build_ipheader(buf, &ctx);
  EXPECT_EQ(ret, 0);

  // ICMP ヘッダーがパケット先頭に構築されること
  struct icmphdr *icmp = (struct icmphdr *)buf;
  EXPECT_EQ(icmp->type, (uint8_t)ICMP_ECHO);
  EXPECT_EQ(icmp->code, 0);
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
  EXPECT_NE(Ping_socket_dgram_ops.extract_icmp, Ping_socket_raw_ops.extract_icmp);
  EXPECT_NE(Ping_socket_dgram_ops.extract_ttl, Ping_socket_raw_ops.extract_ttl);
  EXPECT_NE(Ping_socket_dgram_ops.packet_size, Ping_socket_raw_ops.packet_size);
  EXPECT_NE(Ping_socket_dgram_ops.extra_configure, Ping_socket_raw_ops.extra_configure);
}

// extract_ttl_dgram: cmsg を走査して IP_TTL を取り出す
TEST(VsockDgramOpsTest, ExtractTtlReadsCmsg) {
  alignas(struct cmsghdr) uint8_t cbuf[CMSG_SPACE(sizeof(int))] = {};
  struct msghdr msg = {};
  msg.msg_control = cbuf;
  msg.msg_controllen = sizeof(cbuf);

  struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
  c->cmsg_level = IPPROTO_IP;
  c->cmsg_type = IP_TTL;
  c->cmsg_len = CMSG_LEN(sizeof(int));
  int ttl_val = 57;
  memcpy(CMSG_DATA(c), &ttl_val, sizeof(ttl_val));

  EXPECT_EQ(Ping_socket_dgram_ops.extract_ttl(nullptr, &msg), 57);
}

TEST(VsockDgramOpsTest, ExtractTtlReturnsZeroWhenNoCmsg) {
  struct msghdr msg = {};
  EXPECT_EQ(Ping_socket_dgram_ops.extract_ttl(nullptr, &msg), 0);
}

TEST(VsockDgramOpsTest, ExtractTtlReturnsZeroForNullMsg) {
  EXPECT_EQ(Ping_socket_dgram_ops.extract_ttl(nullptr, nullptr), 0);
}
