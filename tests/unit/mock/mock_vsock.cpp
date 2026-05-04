#include "mock_vsock.h"

#include <cstring>

MockVsockState g_mock_vsock_state = {};

static int mock_build_ipicmp(void *packet, const t_ipheader_ctx *ctx) {
  g_mock_vsock_state.build_ipheader_calls++;
  if (ctx) {
    g_mock_vsock_state.last_seq = ctx->seq;
    g_mock_vsock_state.last_datalen = ctx->datalen;
  }
  if (packet)
    std::memset(packet, 0, sizeof(struct icmphdr));
  return 0;
}

static struct icmphdr *mock_extract_icmp(void *packet, size_t packet_len, int *icmp_len_out) {
  g_mock_vsock_state.extract_icmp_calls++;
  if (icmp_len_out)
    *icmp_len_out = static_cast<int>(packet_len);
  return reinterpret_cast<struct icmphdr *>(packet);
}

static int mock_extract_ttl(void *packet, const struct msghdr *msg) {
  (void)packet;
  (void)msg;
  return 0;
}

static size_t mock_packet_size(size_t datalen) {
  g_mock_vsock_state.packet_size_calls++;
  g_mock_vsock_state.last_packet_size_arg = datalen;
  return sizeof(struct icmphdr) + datalen;
}

static int mock_extra_configure(int fd) {
  (void)fd;
  g_mock_vsock_state.extra_configure_calls++;
  return 0;
}

static int mock_set_ident(int fd, uint16_t ident) {
  (void)fd;
  (void)ident;
  g_mock_vsock_state.set_ident_calls++;
  return 0;
}

extern "C" {
t_ping_socket_ops Mock_socket_ops = {
    .build_ipicmp = mock_build_ipicmp,
    .extract_icmp = mock_extract_icmp,
    .extract_ttl = mock_extract_ttl,
    .packet_size = mock_packet_size,
    .extra_configure = mock_extra_configure,
    .set_ident = mock_set_ident,
};
}

void mock_vsock_reset(void) { std::memset(&g_mock_vsock_state, 0, sizeof(g_mock_vsock_state)); }

void mock_vsock_attach(t_socket_st *st) {
  if (!st)
    return;
  st->fd = 999; /* 擬似 fd: 実 IO は失敗する */
  st->socktype = SOCK_DGRAM;
  st->ops = &Mock_socket_ops;
}
