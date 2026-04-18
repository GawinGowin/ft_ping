#include "vsock/vsock.h"

#include "ping_icmp.h"

int build_packet_dgram(void *packet, const t_build_ctx *ctx) {
  if (packet == NULL || ctx == NULL || ctx->datalen <= 0) {
    return -1;
  }
  struct icmphdr *icmp_hdr = (struct icmphdr *)packet;
  unsigned char *payload = (unsigned char *)packet + sizeof(struct icmphdr);
  ping_icmp_build_echo(icmp_hdr, payload, ctx->seq, ctx->datalen, ctx->ts);
  return 0;
}

struct icmphdr *extract_icmp_dgram(void *packet, size_t packet_len, int *icmp_len_out) {
  *icmp_len_out = packet_len;
  return (struct icmphdr *)packet;
}

int extra_configure_dgram(int fd) {
  (void)fd;
  return 0;
}

size_t packet_size_dgram(size_t datalen) { return sizeof(struct icmphdr) + datalen; }

t_ping_socket_ops Ping_socket_dgram_ops = {
    .build_packet = build_packet_dgram,
    .extract_icmp = extract_icmp_dgram,
    .packet_size = packet_size_dgram,
    .extra_configure = extra_configure_dgram};
