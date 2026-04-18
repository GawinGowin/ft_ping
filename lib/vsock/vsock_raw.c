#include "vsock.h"

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

  icmp_hdr->type = ICMP_ECHO;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->un.echo.id = htons(getpid());
  icmp_hdr->un.echo.sequence = htons(ctx->seq + 1);
  for (size_t i = 0; i < ctx->datalen; i++) {
    payload[i] = (unsigned char)((size_t)i % UCHAR_MAX);
  }
  if (ctx->datalen >= sizeof(*(ctx->ts))) {
    memcpy(payload, ctx->ts, sizeof(*(ctx->ts)));
  }
  size_t packet_size = sizeof(struct icmphdr) + ctx->datalen;
  icmp_hdr->checksum = calculate_checksum((void *)icmp_hdr, packet_size);
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
  packet_icmp->ip.check = calculate_checksum(&packet_icmp->ip, packet_icmp->ip.ihl * 4);
  return 0;
}

struct icmphdr *extract_icmp_raw(void *packet, size_t packet_len, int *icmp_len_out) {
  struct icmphdr *icmp;
  struct iphdr *ip = NULL;
  int icmp_len;

  ip = (struct iphdr *)packet;
  icmp_len = packet_len - (ip->ihl * 4);
  if (icmp_len < 8) {
    return NULL;
  }
  *icmp_len_out = icmp_len;
  icmp = (struct icmphdr *)((char *)packet + (ip->ihl * 4));
  return icmp;
}

int extra_configure_raw(int fd) {
  // IP_HDRINCLオプションの設定（自前でIPヘッダを含める）: これがないとRAWソケットで送信できない
  int hdrincl = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(hdrincl)) < 0) {
    return -1; // error(1, "setsockopt IP_HDRINCL failed: %s\n", strerror(errno));
  }
  return 0;
}

size_t packet_size_raw(size_t datalen) { return sizeof(t_ip_icmp) + datalen; }

t_ping_socket_ops Ping_socket_raw_ops = {
    .build_packet = build_packet_raw,
    .extract_icmp = extract_icmp_raw,
    .packet_size = packet_size_raw,
    .extra_configure = extra_configure_raw};