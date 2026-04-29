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

/* deadline_sec が設定されていれば、count 到達時もスケジューリングしない */
TEST_F(PingScheduleExitTest, DeadlineSetSuppressesScheduling) {
  config.count = 3;
  config.deadline_sec = 10;
  stats.ntransmitted = 3;
  stats.nreceived = 1;
  stats.tmax = 100;

  int result = ping_schedule_exit(&config, &stats, &timer, 5);

  EXPECT_EQ(result, 5);
  EXPECT_EQ(timer.schedule_waittime, 0u);
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
