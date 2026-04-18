#include "vsock.h"

uint16_t calculate_checksum(void *data, size_t len) {
  uint32_t sum = 0;
  uint16_t *ptr = data;

  while (len > 1) {
    sum += *ptr++;
    len -= 2;
  }
  if (len == 1) {
    sum += *(uint8_t *)ptr;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

// TODO: 設定されたオプションパラメータに応じてどちらのソケットで作成するか選択するように
int ping_socket_select(t_socket_st *socket_state) {
  socket_state->fd = -1;
  socket_state->socktype = -1;

  socket_state->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (socket_state->fd >= 0) {
    socket_state->socktype = SOCK_RAW;
    socket_state->ops = &Ping_socket_raw_ops;
    return socket_state->fd;
  }
  // SOCK_DGRAM で作る
  if (errno == EPERM || errno == EACCES) {
    socket_state->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (socket_state->fd >= 0) {
      socket_state->socktype = SOCK_DGRAM;
      socket_state->ops = &Ping_socket_dgram_ops;
      return socket_state->fd;
    }
  }
  return -1;
}