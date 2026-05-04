#include "vsock/vsock.h"

#include "ping_icmp.h"
#include "shared/shared_net.h"

static int set_ip_header(void *packet, const t_ipheader_ctx *ctx);

/* IPヘッダーとICMPヘッダーを構築する */
int build_ipicmp_raw(void *packet, const t_ipheader_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen == 0) {
    return -1;
  }
  set_ip_header(packet, ctx);
  t_ip_icmp *pkt = (t_ip_icmp *)packet;
  unsigned char *payload = (unsigned char *)(pkt + 1);
  ping_icmp_build_echo(&pkt->icmp, payload, ctx->seq, ctx->ident, ctx->datalen, &ctx->ts);
  return 0;
}

static int set_ip_header(void *packet, const t_ipheader_ctx *ctx) {
  t_ip_icmp *pkt = (t_ip_icmp *)packet;

  pkt->ip.version = 4;
  pkt->ip.ihl = 5;
  pkt->ip.tos = (unsigned char)ctx->tos;
  pkt->ip.tot_len = htons(sizeof(t_ip_icmp) + ctx->datalen);
  pkt->ip.id = htons(getpid());
  pkt->ip.frag_off = 0;
  pkt->ip.ttl = (unsigned char)ctx->ttl;
  pkt->ip.protocol = IPPROTO_ICMP;
  pkt->ip.check = 0;
  pkt->ip.saddr = ctx->src.s_addr;
  pkt->ip.daddr = ctx->dst.s_addr;
  pkt->ip.check = inet_checksum(&pkt->ip, pkt->ip.ihl * 4);
  return 0;
}

/* RAW 受信: IPヘッダーをスキップして ICMP ヘッダーへのポインタを返す */
struct icmphdr *extract_icmp_raw(void *packet, size_t packet_len, int *icmp_len_out) {
  struct iphdr *ip = (struct iphdr *)packet;
  int icmp_len = packet_len - (ip->ihl * 4);
  if (icmp_len < 8) {
    return NULL;
  }
  *icmp_len_out = icmp_len;
  return (struct icmphdr *)((char *)packet + (ip->ihl * 4));
}

/* RAW 受信: パケット先頭が IP ヘッダーなので ttl をそのまま読む */
int extract_ttl_raw(void *packet, const struct msghdr *msg) {
  (void)msg;
  if (packet == NULL)
    return 0;
  return ((struct iphdr *)packet)->ttl;
}

int extra_configure_raw(int fd) {
  int hdrincl = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(hdrincl)) < 0) {
    return -1;
  }
  return 0;
}

size_t packet_size_raw(size_t datalen) { return sizeof(t_ip_icmp) + datalen; }

int set_ident_raw(int fd, uint16_t ident) {
  (void)fd;
  (void)ident;
  return 0;
}

t_ping_socket_ops Ping_socket_raw_ops = {
    .build_ipicmp = build_ipicmp_raw,
    .extract_icmp = extract_icmp_raw,
    .extract_ttl = extract_ttl_raw,
    .packet_size = packet_size_raw,
    .extra_configure = extra_configure_raw,
    .set_ident = set_ident_raw};
