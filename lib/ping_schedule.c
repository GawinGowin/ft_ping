#include "ping_schedule.h"
#include "ping_loop.h"

static int ping_schedule_exit_internal(
    t_ping_config *config, t_ping_stats_internal *ctx, t_ping_timer *timer, int next);

int ping_schedule_exit(
    t_ping_config *config, t_ping_stats_internal *ctx, t_ping_timer *timer, int next) {
  if (config->count && ctx->ntransmitted >= config->count) {
    next = ping_schedule_exit_internal(config, ctx, timer, next);
  }
  return next;
}

/**
 * @brief SIGALRM信号による適応的ping終了処理のスケジューリング
 * 
 * 指定されたパケット数の送信完了後、最後の応答パケットを受信するための
 * 適切な待機時間を計算し、SIGALRMタイマーを設定する。
 * `timer->schedule_waittime` により複数回呼び出し時の重複実行を防止する。
 *
 * ## 待機時間決定ロジック
 *
 * ### ケース1: 応答パケット受信済み (`ctx->nreceived > 0`)
 * - 基本待機時間 = `2 × 最大RTT (ctx->tmax)`
 * - 最小保証時間 = `1000 × ping間隔 (config->interval_ms)`
 * - より長い方を採用してネットワーク遅延に対応
 *
 * ### ケース2: 応答パケット未受信 (`ctx->nreceived == 0`)
 * - 待機時間 = `config->lingertime_us` (マイクロ秒単位)
 * - デフォルトの待機戦略を適用
 *
 * ## タイマー制御メカニズム
 * - `setitimer(ITIMER_REAL, ...)` による実時間ベースのタイムアウト
 * - SIGALRMハンドラー経由でのグレースフル終了処理
 * - `timer->schedule_waittime` による初回実行のみの制御
 *
 * @param config  ping設定（count, interval_ms, lingertime_us 等）
 * @param ctx     統計情報（nreceived, tmax 等）
 * @param timer   タイマー状態（schedule_waittime を書き込む）
 * @param next    次回パケット送信までの時間（秒単位）
 * @return 調整された次回送信時間（秒単位）、または元の値
 *
 * @note この関数は初回呼び出し時のみ実際の処理を実行し、
 *       2回目以降は早期リターンによる効率化を図る
 * @note 参考実装: iputils ping の __schedule_exit() 関数
 */
static int ping_schedule_exit_internal(
    t_ping_config *config, t_ping_stats_internal *ctx, t_ping_timer *timer, int next) {
  struct itimerval it;

  if (timer->schedule_waittime)
    return next;
  if (ctx->nreceived) {
    timer->schedule_waittime = 2 * ctx->tmax;
    if (timer->schedule_waittime < 1000 * (unsigned long)config->interval_ms)
      timer->schedule_waittime = 1000 * config->interval_ms;
  } else {
    timer->schedule_waittime = config->lingertime_us;
  }
  if (next < 0 || (unsigned long)next < timer->schedule_waittime / 1000)
    next = timer->schedule_waittime / 1000;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 0;
  it.it_value.tv_sec = timer->schedule_waittime / 1000000;
  it.it_value.tv_usec = timer->schedule_waittime % 1000000;
  setitimer(ITIMER_REAL, &it, NULL);
  return next;
}