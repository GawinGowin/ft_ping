#ifndef ICMP_H
#define ICMP_H

#include <limits.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

#define GET_PACKET_DATA(packet)                                                                    \
  (packet + (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) * 3))

typedef struct s_icmp {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
  uint64_t timestamp;
} __attribute__((packed)) t_icmp; // メモリレイアウトの最適化を無効化

void generate_packet_data(void *packet, int datalen);
void set_timestamp(void *packet);
int create_echo_request_packet(void *packet, uint16_t id, uint16_t seq);
uint16_t calculate_checksum(void *data, int len);

#endif /* ICMP_H */
