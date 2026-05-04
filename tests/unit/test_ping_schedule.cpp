#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "ping_config.h"
#include "ping_loop.h" /* t_ping_timer */
#include "ping_schedule.h"
#include "ping_stats.h"
}

class PingScheduleExitTest : public ::testing::Test {
protected:
  t_ping_config config;
  t_ping_stats_internal stats;
  t_ping_timer timer;

  void SetUp() override {
    ping_config_init(&config);
    memset(&stats, 0, sizeof(stats));
    memset(&timer, 0, sizeof(timer));
  }
};

/* count 未到達のときは next をそのまま返し、待機時間を設定しない */
TEST_F(PingScheduleExitTest, NotReachedCountReturnsNextUnchanged) {
  config.count = 5;
  stats.ntransmitted = 3;

  int result = ping_schedule_exit(&config, &stats, &timer, 7);

  EXPECT_EQ(result, 7);
  EXPECT_EQ(timer.schedule_waittime, 0u);
}

/* deadline_sec が設定されていても、count 到達時は通常通りスケジューリングする
 * （iputils 準拠: -c と -w 併用時はどちらか早く成立した方で終了）。 */
TEST_F(PingScheduleExitTest, DeadlineDoesNotSuppressSchedulingOnCountReached) {
  config.count = 3;
  config.deadline_sec = 10;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000; /* 5秒 */

  ping_schedule_exit(&config, &stats, &timer, 0);

  /* count 到達 + 受信ありなので 2 * tmax がスケジュールされる */
  EXPECT_EQ(timer.schedule_waittime, 2u * 5000000u);
}

/* count 到達 + 受信あり: tmax が大きい場合は schedule_waittime = 2 * tmax */
TEST_F(PingScheduleExitTest, ReachedCountUsesTwiceTmaxWhenLarge) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000; /* 5秒 */

  int result = ping_schedule_exit(&config, &stats, &timer, 0);

  EXPECT_EQ(timer.schedule_waittime, 2u * 5000000u);
  EXPECT_GE(result, 0);
}

/* count 到達 + 受信あり: tmax が小さい場合は最小保証時間 1000*interval_ms を採用 */
TEST_F(PingScheduleExitTest, ReachedCountUsesIntervalFloorWhenTmaxSmall) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 100; /* 100μs と非常に小さい */

  ping_schedule_exit(&config, &stats, &timer, 0);

  EXPECT_EQ(timer.schedule_waittime, 1000u * 1000u);
}

/* count 到達 + 未受信: schedule_waittime = lingertime_us */
TEST_F(PingScheduleExitTest, ReachedCountWithoutReplyUsesLingerTime) {
  config.count = 3;
  config.lingertime_us = 7 * 1000000;
  stats.ntransmitted = 3;
  stats.nreceived = 0;

  ping_schedule_exit(&config, &stats, &timer, 0);

  EXPECT_EQ(timer.schedule_waittime, 7u * 1000000u);
}

/* count == 0（無限モード）: ntransmitted がいくらでもスケジュールしない */
TEST_F(PingScheduleExitTest, CountZeroNeverSchedules) {
  config.count = 0;
  stats.ntransmitted = 1000000;
  stats.nreceived = 1;
  stats.tmax = 5000000;

  int result = ping_schedule_exit(&config, &stats, &timer, 42);

  EXPECT_EQ(result, 42);
  EXPECT_EQ(timer.schedule_waittime, 0u);
}

/* 境界値: ntransmitted == count - 1 ではスケジュールしない */
TEST_F(PingScheduleExitTest, JustBelowCountDoesNotSchedule) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 2; /* count - 1 */
  stats.nreceived = 1;
  stats.tmax = 5000000;

  int result = ping_schedule_exit(&config, &stats, &timer, 11);

  EXPECT_EQ(result, 11);
  EXPECT_EQ(timer.schedule_waittime, 0u);
}

/* 境界値: ntransmitted == count ちょうどでスケジュールが走る */
TEST_F(PingScheduleExitTest, ExactCountTriggersScheduling) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3; /* == count */
  stats.nreceived = 1;
  stats.tmax = 5000000;

  ping_schedule_exit(&config, &stats, &timer, 0);

  EXPECT_NE(timer.schedule_waittime, 0u);
}

/* next が schedule_waittime/1000 より大きいときは next がそのまま返る */
TEST_F(PingScheduleExitTest, LargerNextPreservedAsReturn) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000; /* schedule_waittime = 10_000_000 us → 10_000 ms */

  int next = 99999; /* > 10000 */
  int result = ping_schedule_exit(&config, &stats, &timer, next);

  EXPECT_EQ(result, next);
}

/* next が schedule_waittime/1000 より小さいときは引き上げられる */
TEST_F(PingScheduleExitTest, SmallerNextRaisedToWaittime) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000; /* schedule_waittime = 10_000_000 us → 10_000 ms */

  int result = ping_schedule_exit(&config, &stats, &timer, 100);

  EXPECT_EQ(result, 10000);
}

/* next が負のときも schedule_waittime/1000 に置き換えられる */
TEST_F(PingScheduleExitTest, NegativeNextRaisedToWaittime) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000;

  int result = ping_schedule_exit(&config, &stats, &timer, -1);

  EXPECT_EQ(result, 10000);
}

/* 二重呼び出し: 1回目で設定された schedule_waittime は2回目で変更されない */
TEST_F(PingScheduleExitTest, SecondCallIsIdempotent) {
  config.count = 3;
  config.interval_ms = 1000;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 5000000;

  ping_schedule_exit(&config, &stats, &timer, 0);
  unsigned long first = timer.schedule_waittime;
  EXPECT_NE(first, 0u);

  /* tmax をさらに大きくしても schedule_waittime は変わらない */
  stats.tmax = 99999999;
  ping_schedule_exit(&config, &stats, &timer, 0);

  EXPECT_EQ(timer.schedule_waittime, first);
}
