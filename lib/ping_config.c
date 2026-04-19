#include "ping_config.h"

void ping_config_init(t_ping_config *config) {
  config->datalen = 56;
  config->ttl = 64;
  config->hostname = NULL;
  config->count = 0; // if 0 => inf
  config->interval_ms = 0;
  config->tos = 0;
}
