#include "vsock/vsock.h"

#include "shared/shared_net.h"

static int set_ip_header(void *packet, const t_ipheader_ctx *ctx);

/* IPヘッダーを書く。ICMPの組み立ては呼び出し元（ping_loop.c）が行う */
int build_ipheader_raw(void *packet, const t_ipheader_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen == 0) {
    return -1;
  }
  return set_ip_header(packet, ctx);
}

static int set_ip_header(void *packet, const t_ipheader_ctx *ctx) {
  t_ip_icmp *pkt = (t_ip_icmp *)packet;

  pkt->ip.version = 4;
  pkt->ip.ihl = 5;
  pkt->ip.tos = 0;
  pkt->ip.tot_len = htons(sizeof(t_ip_icmp) + ctx->datalen);
  pkt->ip.id = htons(getpid());
  pkt->ip.frag_off = 0;
  pkt->ip.ttl = 64;
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

int extra_configure_raw(int fd) {
  int hdrincl = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(hdrincl)) < 0) {
    return -1;
  }
  return 0;
}

size_t packet_size_raw(size_t datalen) { return sizeof(t_ip_icmp) + datalen; }

t_ping_socket_ops Ping_socket_raw_ops = {
    .build_ipheader = build_ipheader_raw,
    .extract_icmp = extract_icmp_raw,
    .packet_size = packet_size_raw,
    .extra_configure = extra_configure_raw};
