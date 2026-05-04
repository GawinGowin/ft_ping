#include "vsock/vsock.h"

int ping_socket_select(t_socket_st *socket_state, int need_rawsock) {
  socket_state->fd = -1;
  socket_state->socktype = -1;

  socket_state->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (socket_state->fd >= 0) {
    socket_state->socktype = SOCK_RAW;
    socket_state->ops = &Ping_socket_raw_ops;
    return socket_state->fd;
  }
  // fall back
  if ((errno == EPERM || errno == EACCES) && !need_rawsock) {
    socket_state->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (socket_state->fd >= 0) {
      socket_state->socktype = SOCK_DGRAM;
      socket_state->ops = &Ping_socket_dgram_ops;
      return socket_state->fd;
    }
  }
  return -1;
}