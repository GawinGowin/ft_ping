#include "vsock/vsock.h"

#include <string.h>

#include "ping_icmp.h"

/* DGRAM は IP ヘッダー不要。ICMP ヘッダーのみ構築する */
int build_ipicmp_dgram(void *packet, const t_ipheader_ctx *ctx) {
  if (packet == NULL || ctx == NULL)
    return -1;
  struct icmphdr *icmp_hdr = (struct icmphdr *)packet;
  unsigned char *payload = (unsigned char *)(icmp_hdr + 1);
  ping_icmp_build_echo(icmp_hdr, payload, ctx->seq, ctx->ident, ctx->datalen, &ctx->ts);
  return 0;
}

/* DGRAM 受信: パケット先頭が直接 ICMP ヘッダー */
struct icmphdr *extract_icmp_dgram(void *packet, size_t packet_len, int *icmp_len_out) {
  *icmp_len_out = packet_len;
  return (struct icmphdr *)packet;
}

/* DGRAM 受信: IP ヘッダーは届かない。IP_RECVTTL 由来の cmsg から取り出す。
 * iputils ping4_parse_reply (reference/iputils/ping/ping.c:1667) と同じパターン。 */
int extract_ttl_dgram(void *packet, const struct msghdr *msg) {
  (void)packet;
  if (msg == NULL)
    return 0;
  /* CMSG_NXTHDR は非 const の msghdr* を要求するためキャストする */
  struct msghdr *mut = (struct msghdr *)msg;
  for (struct cmsghdr *c = CMSG_FIRSTHDR(mut); c != NULL; c = CMSG_NXTHDR(mut, c)) {
    if (c->cmsg_level != IPPROTO_IP || c->cmsg_type != IP_TTL)
      continue;
    if (c->cmsg_len < CMSG_LEN(sizeof(int)))
      continue;
    int ttl;
    memcpy(&ttl, CMSG_DATA(c), sizeof(ttl));
    return ttl;
  }
  return 0;
}

int extra_configure_dgram(int fd) {
  (void)fd;
  return 0;
}

size_t packet_size_dgram(size_t datalen) { return sizeof(struct icmphdr) + datalen; }

int set_ident_dgram(int fd, uint16_t ident) {
  struct sockaddr_in source_addr;
  memset(&source_addr, 0, sizeof(source_addr));
  source_addr.sin_family = AF_INET;
  source_addr.sin_port = htons(ident);
  source_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(fd, (struct sockaddr *)&source_addr, sizeof(source_addr)) < 0) {
    return 1;
  }
  return 0;
}

t_ping_socket_ops Ping_socket_dgram_ops = {
    .build_ipicmp = build_ipicmp_dgram,
    .extract_icmp = extract_icmp_dgram,
    .extract_ttl = extract_ttl_dgram,
    .packet_size = packet_size_dgram,
    .extra_configure = extra_configure_dgram,
    .set_ident = set_ident_dgram};
