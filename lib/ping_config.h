#ifndef PING_CONFIG_H
#define PING_CONFIG_H

#include <stdint.h>
#include <stdlib.h>

typedef struct ping_config {
  const char *hostname;
  int datalen;
  int ttl;
  int tos;
  long count;
  int interval_ms;
  int deadline_sec;
  uint32_t lingertime_us;
  uint16_t ident;
  int sndbuf;
  int preload;
  unsigned int opt_adaptive : 1;
  unsigned int opt_flood_poll : 1;
  unsigned int opt_verbose : 1;
  unsigned int opt_ptimeofday : 1;
} t_ping_config;

void ping_config_init(t_ping_config *config);

#endif /* PING_CONFIG_H */
