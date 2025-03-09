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
  struct timespec now;

  t_icmp *icmp = (t_icmp *)packet;
  clock_gettime(CLOCK_MONOTONIC, &now);
  icmp->timestamp = (uint64_t)now.tv_sec * 1000000000ULL + now.tv_nsec;
  return;
}

int create_echo_request_packet(void *packet, uint16_t id, uint16_t seq) {
  struct timespec now;

  t_icmp *icmp = (t_icmp *)packet;

  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->id = htons(id);
  icmp->seq = htons(seq);

  clock_gettime(CLOCK_MONOTONIC, &now);
  icmp->timestamp = (uint64_t)now.tv_sec * 1000000000ULL + now.tv_nsec;

  // この辺は別の関数で行う
  // if (data && datalen > 0) {
  //   memcpy((char *)(packet + 1), data, datalen);
  // }

  // packet->checksum = 0;
  // packet->checksum = calculate_checksum((void *)packet, sizeof(t_icmp) + datalen);
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
