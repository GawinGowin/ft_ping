#include "vsock/vsock.h"

#include "ping_icmp.h"

static int set_ip_header(void *packet, const t_build_ctx *ctx);

int build_packet_raw(void *packet, const t_build_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen <= 0) {
    return -1;
  }
  if (set_ip_header(packet, ctx) != 0) {
    return -1;
  }
  t_ip_icmp *raw_icmp_hdr = (t_ip_icmp *)packet;
  struct icmphdr *icmp_hdr = &raw_icmp_hdr->icmp;
  unsigned char *payload = (unsigned char *)packet + sizeof(t_ip_icmp);

  ping_icmp_build_echo(icmp_hdr, payload, ctx->seq, ctx->datalen, ctx->ts);
  return 0;
}

static int set_ip_header(void *packet, const t_build_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen <= 0) {
    return -1;
  }
  t_ip_icmp *packet_icmp = (t_ip_icmp *)packet;

  packet_icmp->ip.version = 4;
  packet_icmp->ip.ihl = 5;
  packet_icmp->ip.tos = 0;
  packet_icmp->ip.tot_len = htons(sizeof(t_ip_icmp) + ctx->datalen);
  packet_icmp->ip.id = htons(getpid());
  packet_icmp->ip.frag_off = 0;
  packet_icmp->ip.ttl = 64;
  packet_icmp->ip.protocol = IPPROTO_ICMP;
  packet_icmp->ip.check = 0;

  packet_icmp->ip.saddr = ctx->src.s_addr;
  packet_icmp->ip.daddr = ctx->dst.s_addr;
  packet_icmp->ip.check = ping_icmp_checksum(&packet_icmp->ip,
                                              packet_icmp->ip.ihl * 4);
  return 0;
}

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
    .build_packet = build_packet_raw,
    .extract_icmp = extract_icmp_raw,
    .packet_size = packet_size_raw,
    .extra_configure = extra_configure_raw};
