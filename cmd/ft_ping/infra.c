#include "ft_ping.h"

int is_valid_ip_address(char const *const address) {
  struct in_addr ipv4;
  struct in6_addr ipv6;

  if (inet_pton(AF_INET, address, &ipv4) == 1) {
    return 1;
  }
  if (inet_pton(AF_INET6, address, &ipv6) == 1) {
    return 0; // IPv6 is not supported
  }
  return 0;
}
