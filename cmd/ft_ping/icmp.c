#include "icmp.h"

static uint16_t culculateChecksum(void *data, int len);

int createEchoRequestPacket(t_icmp *packet, uint16_t id, uint16_t seq) {
  packet->type = 8;
  packet->code = 0;
  packet->id = id;
  packet->seq = seq; // 1, 2, 3, ...

  packet->checksum = 0;
  packet->checksum = culculateChecksum((void *)packet, sizeof(t_icmp));
  return 0;
}

static uint16_t culculateChecksum(void *data, int len) {
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
