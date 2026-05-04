#ifndef VSOCK_H
#define VSOCK_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/* IP ヘッダー組み立てコンテキスト */
typedef struct ipheader_ctx {
  uint16_t seq;
  uint16_t ident;
  int ttl;
  int tos;
  size_t datalen;
  struct timeval ts;
  struct in_addr src;
  struct in_addr dst;
} t_ipheader_ctx;

typedef struct ping_socket_ops t_ping_socket_ops;

typedef struct socket_st {
  int fd;
  int socktype;
  const t_ping_socket_ops *ops;
} t_socket_st;

/* vtable: RAW と DGRAM の差分のみを抽象化 */
struct ping_socket_ops {
  int (*build_ipicmp)(void *packet, const t_ipheader_ctx *ctx);
  struct icmphdr *(*extract_icmp)(void *packet, size_t packet_len, int *icmp_len_out);
  /* TTL 取得: RAW は packet 先頭の IP ヘッダーから、DGRAM は msg の cmsg から取り出す。
   * 不明なら 0 を返す。 */
  int (*extract_ttl)(void *packet, const struct msghdr *msg);
  size_t (*packet_size)(size_t datalen);
  int (*extra_configure)(int fd);
  int (*set_ident)(int fd, uint16_t ident);
};

/* RAW ソケット専用: IP+ICMP が連続するパケット構造体 */
typedef struct ip_icmp {
  struct iphdr ip;
  struct icmphdr icmp;
} t_ip_icmp;

extern t_ping_socket_ops Ping_socket_raw_ops;
extern t_ping_socket_ops Ping_socket_dgram_ops;

int ping_socket_select(t_socket_st *socket_state, int need_rawsock);

#endif /* VSOCK_H */
