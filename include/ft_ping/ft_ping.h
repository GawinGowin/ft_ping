#ifndef FT_PING_H
#define FT_PING_H

#include <stdint.h>

/* 重複検出システム */
#define MAX_DUP_CHK 0x10000 // 65536個のシーケンス番号を追跡
#define BITMAP_SHIFT 6      // 64bit単位でのビット操作用

typedef uint64_t bitmap_t;

struct rcvd_table {
  bitmap_t bitmap[MAX_DUP_CHK / (sizeof(bitmap_t) * 8)];
};

/* ビット操作マクロ */
#define BITMAP_WORD(tbl, bit) ((tbl)->bitmap[(bit) >> BITMAP_SHIFT])
#define BITMAP_MASK(bit) (((bitmap_t)1) << ((bit) & ((1 << BITMAP_SHIFT) - 1)))

typedef struct ftping_session t_ftping_session; // 前方宣言

typedef struct ftping_config {
  int datalen;
  int ttl;
  int tos;
  long npackets;
  int interval;
  unsigned int opt_verbose;
  unsigned int opt_adaptive;
} t_ftping_config;

typedef struct ftping_stats {
  int ntransmitted;
  int nreceived;
  long tmin;
  long tmax;
  double tsum;
  rcvd_table rcvd_tbl;
} t_ftping_stats;

typedef struct t_ping_session t_ping_session;  /* opaque */

void ftping_init();
void ftping_run();
void ftping_get_stats();
void ftping_stop();
void ftping_cleanup();
void ftping_strerror();

#endif /* FT_PING_H */
