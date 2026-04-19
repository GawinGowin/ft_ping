#ifndef PING_CONFIG_H
#define PING_CONFIG_H

#include <stdlib.h>

typedef struct ping_config {
  const char *hostname;
  int datalen;
  int ttl;
  int tos;
  long count;
  int interval_ms;
  unsigned int opt_adaptive : 1;
  int opt_flood_poll;
} t_ping_config;

void ping_config_init(t_ping_config *config);

#endif /* PING_CONFIG_H */
