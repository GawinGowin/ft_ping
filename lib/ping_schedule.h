#ifndef PING_SCHEDULE_H
#define PING_SCHEDULE_H

#include <stdlib.h>
#include <sys/time.h>

#include "ping_config.h"
#include "ping_stats.h"

typedef struct ping_timer t_ping_timer;

int ping_schedule_exit(
    t_ping_config *config, t_ping_stats_internal *ctx, t_ping_timer *timer, int next);

#endif /* PING_SCHEDULE_H */
