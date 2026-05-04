#include "ping_icmp.h"

#include "shared/shared_net.h"

void ping_icmp_build_echo(
    struct icmphdr *icmp_hdr,
    unsigned char *payload,
    uint16_t seq,
    uint16_t ident,
    size_t datalen,
    const struct timeval *ts) {
  icmp_hdr->type = ICMP_ECHO;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->un.echo.id = htons(ident);
  icmp_hdr->un.echo.sequence = htons(seq + 1);
  for (size_t i = 0; i < datalen; i++)
    payload[i] = (unsigned char)(i % UCHAR_MAX);
  if (datalen >= sizeof(*ts))
    memcpy(payload, ts, sizeof(*ts));
  icmp_hdr->checksum = inet_checksum(icmp_hdr, sizeof(struct icmphdr) + datalen);
}
