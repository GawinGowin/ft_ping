#ifndef VSOCK_H
#define VSOCK_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct ip_icmp {
  struct iphdr ip;
  struct icmphdr icmp;
} t_ip_icmp;

typedef struct build_ctx {
  uint16_t seq;
  size_t datalen;
  struct timeval *ts;
  struct in_addr src;
  struct in_addr dst;
} t_build_ctx;

typedef struct ping_socket_ops t_ping_socket_ops;

typedef struct socket_st {
  int fd;
  int socktype;
  const t_ping_socket_ops *ops;
} t_socket_st;

typedef struct ping_socket_ops {
  int (*build_packet)(void *packet, const t_build_ctx *ctx);
  struct icmphdr *(*extract_icmp)(void *packet, size_t packet_len, int *icmp_len_out);
  size_t (*packet_size)(size_t datalen);
  int (*extra_configure)(int fd);
} t_ping_socket_ops;

extern t_ping_socket_ops Ping_socket_raw_ops;
extern t_ping_socket_ops Ping_socket_dgram_ops;

int ping_socket_select(t_socket_st *socket_state);

#endif /* VSOCK_H */
