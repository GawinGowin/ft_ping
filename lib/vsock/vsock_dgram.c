#include "vsock/vsock.h"

/* DGRAM は IP ヘッダーを自前で書かない。no-op */
int build_ipheader_dgram(void *packet, const t_ipheader_ctx *ctx) {
  (void)packet;
  (void)ctx;
  return 0;
}

/* DGRAM 受信: パケット先頭が直接 ICMP ヘッダー */
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
    .build_ipheader = build_ipheader_dgram,
    .extract_icmp = extract_icmp_dgram,
    .packet_size = packet_size_dgram,
    .extra_configure = extra_configure_dgram};
