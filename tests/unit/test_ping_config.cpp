#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "ping_config.h"
}

class PingConfigInitTest : public ::testing::Test {
protected:
  t_ping_config config;

  void SetUp() override {
    /* 0xFF で塗り潰してから ping_config_init() を呼ぶ。デフォルト値が
     * 確実に書き込まれていることを検証するため。 */
    memset(&config, 0xFF, sizeof(config));
    ping_config_init(&config);
  }
};

TEST_F(PingConfigInitTest, DataLenIsDefault) { EXPECT_EQ(config.datalen, 56); }

TEST_F(PingConfigInitTest, TtlIsDefault) { EXPECT_EQ(config.ttl, 64); }

TEST_F(PingConfigInitTest, CountIsZeroMeaningInfinite) { EXPECT_EQ(config.count, 0); }

TEST_F(PingConfigInitTest, IntervalMsIsDefault) { EXPECT_EQ(config.interval_ms, 1000); }

TEST_F(PingConfigInitTest, TosIsZero) { EXPECT_EQ(config.tos, 0); }

TEST_F(PingConfigInitTest, DeadlineSecIsZero) { EXPECT_EQ(config.deadline_sec, 0); }

TEST_F(PingConfigInitTest, LingerTimeIsTenSeconds) {
  EXPECT_EQ(config.lingertime_us, 10u * 1000000u);
}

TEST_F(PingConfigInitTest, AdaptiveFlagIsOff) { EXPECT_EQ(config.opt_adaptive, 0u); }

TEST_F(PingConfigInitTest, FloodPollFlagIsOff) { EXPECT_EQ(config.opt_flood_poll, 0u); }

TEST_F(PingConfigInitTest, IdentIsZero) { EXPECT_EQ(config.ident, 0); }

TEST_F(PingConfigInitTest, SndBufIsZero) { EXPECT_EQ(config.sndbuf, 0); }

TEST_F(PingConfigInitTest, PreloadIsOne) { EXPECT_EQ(config.preload, 0); }

TEST_F(PingConfigInitTest, HostnameIsNull) { EXPECT_EQ(config.hostname, nullptr); }
