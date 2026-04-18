#ifndef PING_ICMP_H
#define PING_ICMP_H

#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

uint16_t ping_icmp_checksum(void *data, size_t len);
void ping_icmp_build_echo(
    struct icmphdr *icmp_hdr,
    unsigned char *payload,
    uint16_t seq,
    size_t datalen,
    const struct timeval *ts);

#endif /* PING_ICMP_H */
