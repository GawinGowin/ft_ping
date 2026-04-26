#ifndef PING_STATS_H
#define PING_STATS_H

#include <stdint.h>
#include "ft_ping/ft_ping.h"

typedef struct ping_stats_internal {
  int ntransmitted;
  int nreceived;

  long nrepeats;
  long nchecksum;
  long nerrors;

  long tmin;
  long tmax;
  double tsum;
  double tsum2;
  uint64_t rtt;
  int pipesize;

  struct rcvd_table rcvd_tbl;
  unsigned int timing : 1;

} t_ping_stats_internal;

void ping_stats_gather(t_ping_stats_internal *stats, uint16_t seq, long triptime, int is_duplicate);
void ping_stats_finish(const t_ping_stats_internal *stats, const char *hostname, int interval_ms);
void ping_stats_rcvd_set(t_ping_stats_internal *stats, uint16_t seq);
bitmap_t ping_stats_rcvd_test(const t_ping_stats_internal *stats, uint16_t seq);

#endif /* PING_STATS_H */
