#include "ft_ping.h"

static void dns_lookup(const char *hostname, struct sockaddr_in *addr);

int init(t_ping_state *state, char **argv) {
  state->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (state->sockfd < 0)
    error(1, "socket failed\n");
  int sndbuf = 64 * 1024; // 64KB send buffer size
  if (setsockopt(state->sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
    error(1, "setsockopt failed\n");
  }
  char *target = *argv;
  // アドレスの名前解決
  dns_lookup(target, &state->whereto);
  (void)target;
  // bzero((void *)&state->whereto, sizeof(state->whereto));
  // state->whereto.sin_family = AF_INET;
  // if (inet_aton(target, &state->whereto.sin_addr) == 1) { // 数値形式のアドレス
  //   struct in_addr ipv6;
  //   if (inet_pton(AF_INET6, target, &ipv6) == 1) { // IPv6 is not supported
  //     error(2, "IPv6 is not supported\n");
  //   }
  //   state->hostname = target;

  // } else { // 名前解決が必要なアドレス
  // }
  return (0);
}

static void dns_lookup(const char *hostname, struct sockaddr_in *addr) {
  struct addrinfo hints = {
      .ai_family = AF_INET,      /* Allow only IPv4 */
      .ai_socktype = SOCK_DGRAM, /* Datagram socket */
      .ai_flags = AI_PASSIVE,    /* For wildcard IP address */
      .ai_protocol = 0,          /* Any protocol */
      .ai_canonname = NULL,
      .ai_addr = NULL,
      .ai_next = NULL,
  };
  struct addrinfo *result;
  int ret = getaddrinfo(hostname, NULL, &hints, &result);
  if (ret != 0) {
    error(1, "getaddrinfo failed\n");
  }
  struct sockaddr_in *addr_in = (struct sockaddr_in *)result->ai_addr;
  memcpy(addr, addr_in, sizeof(struct sockaddr_in));
  freeaddrinfo(result);
}

void cleanup(int status, void *state) {
  (void)status;
  t_ping_state *st = (t_ping_state *)state;
  close(st->sockfd);
}
