#include "vsock.h"

int build_packet_dgram(void *packet, const t_build_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen <= 0) {
    return -1;
  }
  struct icmphdr *icmp_hdr = (struct icmphdr *)packet;
  unsigned char *payload = (unsigned char *)packet + sizeof(struct icmphdr);
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

int extract_icmp_dgram(void *packet, size_t packet_len, int *icmp_len_out) {
  *icmp_len_out = packet_len;
  return (struct icmphdr *)packet;
}

int extra_configure_dgram(int fd) {
  (void)fd;
  return 0;
} // dgram ソケットでは不要

size_t packet_size_dgram(size_t datalen) { return sizeof(struct icmphdr) + datalen; }

t_ping_socket_ops Ping_socket_dgram_ops = {
    .build_packet = build_packet_dgram,
    .extract_icmp = extract_icmp_dgram,
    .packet_size = packet_size_dgram,
    .extra_configure = extra_configure_dgram};
