#ifndef PING_SCHEDULE_H
#define PING_SCHEDULE_H

#include <stdlib.h>
#include <sys/time.h>

#include "ping_config.h"
#include "ping_stats.h"

int schedule_exit(t_ping_config *config, t_ping_stats_internal *ctx, int next);

#endif /* PING_SCHEDULE_H */
