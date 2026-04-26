#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

/* 重複検出システム */
#define MAX_DUP_CHK 0x10000 // 65536個のシーケンス番号を追跡
#define BITMAP_SHIFT 6      // 64bit単位でのビット操作用

typedef uint64_t bitmap_t;

typedef struct rcvd_table {
  bitmap_t bitmap[MAX_DUP_CHK / (sizeof(bitmap_t) * 8)];
} rcvd_table;

/* ビット操作マクロ */
#define BITMAP_WORD(tbl, bit) ((tbl)->bitmap[(bit) >> BITMAP_SHIFT])
#define BITMAP_MASK(bit) (((bitmap_t)1) << ((bit) & ((1 << BITMAP_SHIFT) - 1)))

typedef struct ping_config {
  const char *hostname;
  int datalen;
  int ttl;
  int tos;
  long count;
  int interval_ms;
  int deadline_sec;
  uint32_t lingertime_us;
  uint16_t ident;
  int sndbuf;
  int preload;
  unsigned int opt_adaptive : 1;
  unsigned int opt_flood_poll : 1;
  unsigned int opt_verbose : 1;
  unsigned int opt_ptimeofday : 1;
} t_ping_config;

void error(int status, const char *format, ...);

/* 1パケット応答イベント。lib/ → tool_output コールバックの引数。 */
typedef struct ftping_reply {
  int bytes;                /* ICMP ヘッダー＋データ部のサイズ */
  struct in_addr from_addr; /* 送信元 IP */
  uint16_t seq;             /* 受信した ICMP シーケンス（ホスト順） */
  long triptime_us;         /* RTT [μs]、未測定なら -1 */
  int is_duplicate;         /* 重複なら 1 */
  struct timeval recv_time; /* gettimeofday(recv 時)。-D 用 */
} t_ftping_reply;

/* 最終統計用の DTO。tool_output_finish が消費する。 */
typedef struct ftping_summary {
  const char *hostname;
  int interval_ms;
  int ntransmitted;
  int nreceived;
  long nrepeats;
  long nchecksum;
  long nerrors;
  long tmin;    /* μs */
  long tmax;    /* μs */
  double tsum;  /* μs */
  double tsum2; /* μs² */
  int timing;   /* RTT 計測有効なら 1 */
} t_ftping_summary;

typedef void (*t_ftping_reply_cb)(const t_ftping_reply *reply, void *ctx);

typedef struct ping_session t_ping_session; /* opaque */

/* ── 公開API ── */
void ftping_config_init(t_ping_config *config);
t_ping_session *ftping_init(const t_ping_config *config, const char *target);
void ftping_set_reply_handler(t_ping_session *session, t_ftping_reply_cb cb, void *ctx);
void ftping_run(t_ping_session *session);
t_ftping_summary ftping_get_summary(const t_ping_session *session);
size_t ftping_get_packet_size(const t_ping_session *session);
struct in_addr ftping_get_target_addr(const t_ping_session *session);
void ftping_stop(t_ping_session *session);
void ftping_cleanup(t_ping_session *session);

#endif /* FT_PING_H */
