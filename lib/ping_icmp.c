#include "ping_icmp.h"

#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

uint16_t ping_icmp_checksum(void *data, size_t len) {
  uint32_t sum = 0;
  uint16_t *ptr = data;

  while (len > 1) {
    sum += *ptr++;
    len -= 2;
  }
  if (len == 1)
    sum += *(uint8_t *)ptr;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

void ping_icmp_build_echo(
    struct icmphdr *icmp_hdr,
    unsigned char *payload,
    uint16_t seq,
    size_t datalen,
    const struct timeval *ts) {
  icmp_hdr->type = ICMP_ECHO;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->un.echo.id = htons(getpid());
  icmp_hdr->un.echo.sequence = htons(seq + 1);
  for (size_t i = 0; i < datalen; i++)
    payload[i] = (unsigned char)(i % UCHAR_MAX);
  if (datalen >= sizeof(*ts))
    memcpy(payload, ts, sizeof(*ts));
  icmp_hdr->checksum = ping_icmp_checksum(icmp_hdr, sizeof(struct icmphdr) + datalen);
}
