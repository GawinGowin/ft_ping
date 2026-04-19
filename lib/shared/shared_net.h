#ifndef SHARED_NET_H
#define SHARED_NET_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET IPPROTO_IP
#endif

#define SCHINT(a) (((a) <= MIN_INTERVAL_MS) ? MIN_INTERVAL_MS : (a))
#define MIN_INTERVAL_MS 10

#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void dns_lookup(const char *hostname, struct sockaddr_in *addr);
int send_packet(void *packet, size_t packet_size, int sockfd, struct sockaddr_in *whereto);
void get_source_address(struct sockaddr_in *src, struct sockaddr_in *dest, const char *device);
void configure_socket_timeouts(int sockfd, int interval, int *opt_flood_poll);
int is_ipv6_address(const char *addr);
uint16_t inet_checksum(void *data, size_t len);

#endif /* SHARED_NET_H */
