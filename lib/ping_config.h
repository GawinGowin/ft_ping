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
} ping_config_t;

void ping_config_init(ping_config_t *config);

#endif /* PING_CONFIG_H */
