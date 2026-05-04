#include "ping_config.h"

void ping_config_init(t_ping_config *config) {
  config->datalen = 56;
  config->ttl = 64;
  config->hostname = NULL;
  config->count = 0; // if 0 => inf
  config->interval_ms = 1000;
  config->tos = 0;
  config->deadline_sec = 0;
  config->lingertime_us = 10 * 1000000;
  config->opt_adaptive = 0;
  config->opt_flood_poll = 0;
  config->ident = 0;
  config->sndbuf = 0;
  config->preload = 0;
}
