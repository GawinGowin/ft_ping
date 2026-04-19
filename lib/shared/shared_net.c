#include "shared/shared_net.h"
#include "shared/shared_error.h"

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

void get_source_address(struct sockaddr_in *src, struct sockaddr_in *dest, const char *device) {
  memset(src, 0, sizeof(*src));
  int probe_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (probe_fd < 0) {
    error(2, "socket creation failed");
  }

  if (device) {
    if (setsockopt(probe_fd, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device) + 1) == -1) {
      error(2, "SO_BINDTODEVICE failed");
    }
  }
  struct sockaddr_in dst = *dest;
  dst.sin_port = htons(1025);
  if (connect(probe_fd, (struct sockaddr *)&dst, sizeof(dst)) == -1) {
    error(2, "connect failed");
  }
  socklen_t alen = sizeof(*src);
  if (getsockname(probe_fd, (struct sockaddr *)src, &alen) == -1) {
    error(2, "getsockname failed");
  }
  src->sin_port = 0;
  close(probe_fd);
  return;
}

void configure_socket_timeouts(int sockfd, int interval, int *opt_flood_poll) {
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (interval < 1000) {
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * SCHINT(interval);
  }
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
  tv.tv_sec = SCHINT(interval) / 1000;
  tv.tv_usec = 1000 * (SCHINT(interval) % 1000);
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv))) {
    *opt_flood_poll = 1;
  }
}

int is_ipv6_address(const char *addr) {
  if (!addr) {
    return 0;
  }
  struct in6_addr ipv6_addr;
  return inet_pton(AF_INET6, addr, &ipv6_addr) == 1;
}

uint16_t inet_checksum(void *data, size_t len) {
  uint32_t sum = 0;
  uint16_t *ptr = data;
  while (len > 1) { sum += *ptr++; len -= 2; }
  if (len == 1) sum += *(uint8_t *)ptr;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}
