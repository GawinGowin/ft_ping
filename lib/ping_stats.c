#include "ping_stats.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>

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

void ping_stats_finish(
    const t_ping_stats_internal *stats,
    const char *hostname,
    int interval_ms) {

  printf("\n--- %s ping statistics ---\n", hostname);

  printf("%d packets transmitted, %d received", stats->ntransmitted, stats->nreceived);

  if (stats->nrepeats)
    printf(", +%ld duplicates", stats->nrepeats);
  if (stats->nchecksum)
    printf(", +%ld corrupted", stats->nchecksum);
  if (stats->nerrors)
    printf(", +%ld errors", stats->nerrors);

  if (stats->ntransmitted) {
    double loss =
        ((double)(stats->ntransmitted - stats->nreceived) * 100.0) / stats->ntransmitted;
    printf(", %.1f%% packet loss", loss);
  }

  printf(", time %dms\n", stats->ntransmitted * interval_ms);

  if (stats->nreceived && stats->timing) {
    long total = stats->nreceived + stats->nrepeats;
    long avg_rtt = stats->tsum / total;

    long long variance;
    if (stats->tsum < INT_MAX) {
      variance = (stats->tsum2 - ((stats->tsum * stats->tsum) / total)) / total;
    } else {
      variance = (stats->tsum2 / total) - (avg_rtt * avg_rtt);
    }

    double std_dev = 0.0;
    if (variance > 0) {
      double x = variance;
      double prev;
      do {
        prev = x;
        x = (x + variance / x) / 2.0;
      } while (x < prev && (prev - x) > 0.001);
      std_dev = x;
    }

    printf(
        "rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
        stats->tmin / 1000.0, avg_rtt / 1000.0,
        stats->tmax / 1000.0, std_dev / 1000.0);
  }
}

void ping_stats_rcvd_set(t_ping_stats_internal *stats, uint16_t seq) {
  unsigned bit = seq % MAX_DUP_CHK;
  BITMAP_WORD(&stats->rcvd_tbl, bit) |= BITMAP_MASK(bit);
}

bitmap_t ping_stats_rcvd_test(const t_ping_stats_internal *stats, uint16_t seq) {
  unsigned bit = seq % MAX_DUP_CHK;
  return BITMAP_WORD(&stats->rcvd_tbl, bit) & BITMAP_MASK(bit);
}
