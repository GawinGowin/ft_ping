#ifndef PING_ICMP_H
#define PING_ICMP_H

#include <limits.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

void ping_icmp_build_echo(
    struct icmphdr *icmp_hdr,
    unsigned char *payload,
    uint16_t seq,
    uint16_t ident,
    size_t datalen,
    const struct timeval *ts);

#endif /* PING_ICMP_H */
