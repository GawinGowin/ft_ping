#ifndef PING_STATS_H
#define PING_STATS_H

#include <stdint.h>

/* 重複検出システム */
#define MAX_DUP_CHK 0x10000
#define BITMAP_SHIFT 6

typedef uint64_t bitmap_t;

struct rcvd_table {
  bitmap_t bitmap[MAX_DUP_CHK / (sizeof(bitmap_t) * 8)];
};

/* ビット操作マクロ */
#define BITMAP_WORD(tbl, bit) ((tbl)->bitmap[(bit) >> BITMAP_SHIFT])
#define BITMAP_MASK(bit) (((bitmap_t)1) << ((bit) & ((1 << BITMAP_SHIFT) - 1)))

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
