#include "ping_stats.h"

#include <stdint.h>
#include <string.h>

void ping_stats_gather(
    t_ping_stats_internal *stats,
    uint16_t seq __attribute__((__unused__)),
    long triptime,
    int is_duplicate) {

  if (is_duplicate) {
    stats->nrepeats++;
    return;
  }

  stats->nreceived++;

  if (stats->timing && triptime >= 0) {
    stats->tsum += triptime;
    stats->tsum2 += (double)((long long)triptime * (long long)triptime);

    if (triptime < stats->tmin)
      stats->tmin = triptime;
    if (triptime > stats->tmax)
      stats->tmax = triptime;

    if (!stats->rtt)
      stats->rtt = ((uint64_t)triptime) * 8;
    else
      stats->rtt += triptime - stats->rtt / 8;
  }

  int pipe = stats->ntransmitted - stats->nreceived;
  if (pipe > stats->pipesize)
    stats->pipesize = pipe;
}

void ping_stats_compute_summary(
    const t_ping_stats_internal *stats,
    const char *hostname,
    int interval_ms,
    t_ftping_summary *out) {
  if (!out)
    return;
  memset(out, 0, sizeof(*out));
  if (!stats)
    return;
  out->hostname = hostname;
  out->interval_ms = interval_ms;
  out->ntransmitted = stats->ntransmitted;
  out->nreceived = stats->nreceived;
  out->nrepeats = stats->nrepeats;
  out->nchecksum = stats->nchecksum;
  out->nerrors = stats->nerrors;
  out->tmin = stats->tmin;
  out->tmax = stats->tmax;
  out->tsum = stats->tsum;
  out->tsum2 = stats->tsum2;
  out->timing = stats->timing;
}

void ping_stats_rcvd_set(t_ping_stats_internal *stats, uint16_t seq) {
  unsigned bit = seq % MAX_DUP_CHK;
  BITMAP_WORD(&stats->rcvd_tbl, bit) |= BITMAP_MASK(bit);
}

bitmap_t ping_stats_rcvd_test(const t_ping_stats_internal *stats, uint16_t seq) {
  unsigned bit = seq % MAX_DUP_CHK;
  return BITMAP_WORD(&stats->rcvd_tbl, bit) & BITMAP_MASK(bit);
}
