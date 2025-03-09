#include "icmp.h"

void generate_packet_data(void *packet, int datalen) {
  if (packet == NULL || datalen <= 0) {
    return;
  }
  unsigned char *data = GET_PACKET_DATA(packet);
  for (int i = 0; i < datalen; i++) {
    data[i] = (unsigned char)i % UCHAR_MAX;
  }
  return;
}

void set_timestamp(void *packet) {
  struct timeval tmp_tv;
  gettimeofday(&tmp_tv, NULL);
  t_icmp *icpm = (t_icmp *)packet;
  memcpy(&icpm->timestamp, &tmp_tv, sizeof(tmp_tv));
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
