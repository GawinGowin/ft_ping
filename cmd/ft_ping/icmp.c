#include "icmp.h"

void generate_packet_data(void *packet, size_t datalen) {
  if (packet == NULL || datalen <= 0) {
    return;
  }
  unsigned char *data = GET_PACKET_DATA(packet);
  for (size_t i = 0; i < datalen; i++) {
    data[i] = (unsigned char)((size_t)i % UCHAR_MAX);
  }
  return;
}

void set_timestamp(void *packet, size_t datalen, struct timeval *timestamp) {
  if (datalen < sizeof(*timestamp)) {
    return;
  }
  t_icmp *icpm = (t_icmp *)packet;
  memcpy(&icpm->timestamp, timestamp, sizeof(*timestamp));
  return;
}

int create_echo_request_packet(void *packet, uint16_t id, uint16_t seq) {
  t_icmp *icmp = (t_icmp *)packet;

  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->id = htons(id);
  icmp->seq = htons(seq);
  return 0;
}

uint16_t calculate_checksum(void *data, int len) {
  uint32_t sum = 0;
  uint16_t *ptr = data;

  while (len > 1) {
    sum += *ptr++;
    len -= 2;
  }
  if (len == 1) {
    sum += *(uint8_t *)ptr;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}
