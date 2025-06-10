#include "ft_ping.h"

int is_ipv6_address(const char *addr) {
  if (!addr) {
    return 0;
  }
  struct in6_addr ipv6_addr;
  return inet_pton(AF_INET6, addr, &ipv6_addr) == 1;
}

int create_socket_with_fallback(void) {
  int sockfd;
  // SOCK_RAW を作るにはroot権限が必要なことがある
  sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd >= 0) {
    return sockfd;
  }
  // SOCK_DGRAM で作る
  if (errno == EPERM || errno == EACCES) {
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sockfd >= 0) {
      return sockfd;
    }
  }
  return -1;
}

void dns_lookup(const char *hostname, struct sockaddr_in *addr) {
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
  if (ret != 0 || !result) {
    error(1, "getaddrinfo failed: %s", gai_strerror(ret));
  }

  struct sockaddr_in *addr_in = (struct sockaddr_in *)result->ai_addr;
  memcpy(addr, addr_in, sizeof(struct sockaddr_in));
  freeaddrinfo(result);
}

int send_packet(void *packet, size_t packet_size, int sockfd, struct sockaddr_in *whereto) {
  int cc = sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)whereto, sizeof(*whereto));
  if (cc < 0) {
    return -1;
  }
  return cc;
}
