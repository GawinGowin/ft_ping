#include "ft_ping.h"

int create_socket(t_socket_st *socket_state) {
  socket_state->fd = -1;
  socket_state->socktype = -1;
  socket_state->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (socket_state->fd >= 0) {
    socket_state->socktype = SOCK_RAW;
    return socket_state->fd;
  }
  if (errno == EPERM || errno == EACCES) {
    socket_state->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (socket_state->fd >= 0) {
      socket_state->socktype = SOCK_DGRAM;
      return socket_state->fd;
    }
  }
  return -1;
}
