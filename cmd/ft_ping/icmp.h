#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>

typedef struct s_icmp {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
  // data
  uint64_t timestamp;
} t_icmp;

#endif /* ICMP_H */
