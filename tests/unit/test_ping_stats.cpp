#include <climits>
#include <gtest/gtest.h>
#include <string>

extern "C" {
#include "ping_stats.h"
}

/* tmin の初期値は LONG_MAX が慣例 */
static t_ping_stats_internal make_stats(int timing = 1) {
  t_ping_stats_internal s = {};
  s.tmin = LONG_MAX;
  s.timing = timing;
  return s;
}

/* ─── rcvd_set / rcvd_test ─────────────────────────────── */

class RcvdTableTest : public ::testing::Test {
protected:
  t_ping_stats_internal stats = {};
};

TEST_F(RcvdTableTest, UnsetBitReturnsFalse) {
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 0), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 1), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 100), 0u);
}

TEST_F(RcvdTableTest, SetBitReturnsNonZero) {
  ping_stats_rcvd_set(&stats, 42);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 42), 0u);
}

TEST_F(RcvdTableTest, SetDoesNotAffectOtherBits) {
  ping_stats_rcvd_set(&stats, 42);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 41), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 43), 0u);
}

TEST_F(RcvdTableTest, BoundarySeqZero) {
  ping_stats_rcvd_set(&stats, 0);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 0), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 1), 0u);
}

TEST_F(RcvdTableTest, BoundarySeqMax) {
  uint16_t seq = MAX_DUP_CHK - 1;
  ping_stats_rcvd_set(&stats, seq);
  EXPECT_NE(ping_stats_rcvd_test(&stats, seq), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, seq - 1), 0u);
}

/* seq は MAX_DUP_CHK(65536) でラップアラウンドする。
   uint16_t の最大は 65535 なので、0 と同じビットになる seq=0 で確認する */
TEST_F(RcvdTableTest, SeqWrapsAroundAtMaxDupChk) {
  ping_stats_rcvd_set(&stats, 0);
  /* 0 % 65536 == 0 なので seq=0 のビットが立つ */
  EXPECT_NE(ping_stats_rcvd_test(&stats, 0), 0u);
  /* 未設定の seq=1 は 0 */
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 1), 0u);
}

TEST_F(RcvdTableTest, MultipleBitsIndependent) {
  ping_stats_rcvd_set(&stats, 10);
  ping_stats_rcvd_set(&stats, 100);
  ping_stats_rcvd_set(&stats, 1000);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 10), 0u);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 100), 0u);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 1000), 0u);
  EXPECT_EQ(ping_stats_rcvd_test(&stats, 11), 0u);
}

TEST_F(RcvdTableTest, SetIsIdempotent) {
  ping_stats_rcvd_set(&stats, 7);
  ping_stats_rcvd_set(&stats, 7);
  EXPECT_NE(ping_stats_rcvd_test(&stats, 7), 0u);
}

/* ─── ping_stats_gather ─────────────────────────────────── */

class GatherTest : public ::testing::Test {
protected:
  t_ping_stats_internal stats = make_stats();
};

TEST_F(GatherTest, NormalPacketIncrementsNreceived) {
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, 10000, 0);
  EXPECT_EQ(stats.nreceived, 1);
  EXPECT_EQ(stats.nrepeats, 0);
}

TEST_F(GatherTest, DuplicateIncrementsNrepeatsOnly) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 10000, 0);
  ping_stats_gather(&stats, 1, 10000, 1);
  EXPECT_EQ(stats.nreceived, 1);
  EXPECT_EQ(stats.nrepeats, 1);
}

TEST_F(GatherTest, DuplicateSkipsRttUpdate) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 50000, 0);
  double tsum_before = stats.tsum;
  ping_stats_gather(&stats, 1, 99999, 1);
  EXPECT_EQ(stats.tsum, tsum_before);
}

TEST_F(GatherTest, RttMinUpdates) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 50000, 0);
  ping_stats_gather(&stats, 2, 30000, 0);
  EXPECT_EQ(stats.tmin, 30000);
}

TEST_F(GatherTest, RttMaxUpdates) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 30000, 0);
  ping_stats_gather(&stats, 2, 50000, 0);
  EXPECT_EQ(stats.tmax, 50000);
}

TEST_F(GatherTest, RttSumAccumulates) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 10000, 0);
  ping_stats_gather(&stats, 2, 20000, 0);
  EXPECT_DOUBLE_EQ(stats.tsum, 30000.0);
}

TEST_F(GatherTest, RttSum2AccumulatesSquares) {
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, 1000, 0);
  EXPECT_DOUBLE_EQ(stats.tsum2, 1000.0 * 1000.0);
}

TEST_F(GatherTest, EwmaFirstPacket) {
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, 1000, 0);
  EXPECT_EQ(stats.rtt, (uint64_t)1000 * 8);
}

TEST_F(GatherTest, EwmaSubsequentPacket) {
  stats.ntransmitted = 2;
  ping_stats_gather(&stats, 1, 1000, 0);
  uint64_t rtt_after_first = stats.rtt;
  ping_stats_gather(&stats, 2, 2000, 0);
  EXPECT_EQ(stats.rtt, rtt_after_first + 2000 - rtt_after_first / 8);
}

TEST_F(GatherTest, TimingOffSkipsRtt) {
  stats = make_stats(/*timing=*/0);
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, 50000, 0);
  EXPECT_EQ(stats.tsum, 0.0);
  EXPECT_EQ(stats.tmin, LONG_MAX);
  EXPECT_EQ(stats.tmax, 0);
  EXPECT_EQ(stats.rtt, 0u);
}

TEST_F(GatherTest, NegativeTriptimeSkipsRtt) {
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, -1, 0);
  EXPECT_EQ(stats.nreceived, 1);
  EXPECT_EQ(stats.tsum, 0.0);
  EXPECT_EQ(stats.tmin, LONG_MAX);
}

TEST_F(GatherTest, ZeroTriptimeIsValid) {
  stats.ntransmitted = 1;
  ping_stats_gather(&stats, 1, 0, 0);
  EXPECT_EQ(stats.tmin, 0);
  EXPECT_EQ(stats.tmax, 0);
  EXPECT_DOUBLE_EQ(stats.tsum, 0.0);
}

TEST_F(GatherTest, PipesizeUpdates) {
  stats.ntransmitted = 5;
  ping_stats_gather(&stats, 1, 1000, 0); /* pipe = 5 - 1 = 4 */
  EXPECT_EQ(stats.pipesize, 4);
}

TEST_F(GatherTest, PipesizeDoesNotDecrease) {
  stats.ntransmitted = 5;
  ping_stats_gather(&stats, 1, 1000, 0); /* pipe = 4 */
  stats.ntransmitted = 6;
  ping_stats_gather(&stats, 2, 1000, 0); /* pipe = 6 - 2 = 4, pipesize stays 4 */
  EXPECT_EQ(stats.pipesize, 4);
}

/* ─── ping_stats_compute_summary ────────────────────────── */

class ComputeSummaryTest : public ::testing::Test {
protected:
  t_ping_stats_internal stats = {};

  t_ftping_summary compute(int interval_ms = 1000) {
    t_ftping_summary out{};
    ping_stats_compute_summary(&stats, "example.com", interval_ms, &out);
    return out;
  }
};

TEST_F(ComputeSummaryTest, HostnameAndIntervalCopied) {
  auto s = compute(/*interval_ms=*/250);
  EXPECT_STREQ(s.hostname, "example.com");
  EXPECT_EQ(s.interval_ms, 250);
}

TEST_F(ComputeSummaryTest, ZeroPackets) {
  auto s = compute();
  EXPECT_EQ(s.ntransmitted, 0);
  EXPECT_EQ(s.nreceived, 0);
}

TEST_F(ComputeSummaryTest, PacketCountsCopied) {
  stats.ntransmitted = 5;
  stats.nreceived = 4;
  auto s = compute();
  EXPECT_EQ(s.ntransmitted, 5);
  EXPECT_EQ(s.nreceived, 4);
}

TEST_F(ComputeSummaryTest, RepeatChecksumErrorsCopied) {
  stats.nrepeats = 2;
  stats.nchecksum = 3;
  stats.nerrors = 4;
  auto s = compute();
  EXPECT_EQ(s.nrepeats, 2);
  EXPECT_EQ(s.nchecksum, 3);
  EXPECT_EQ(s.nerrors, 4);
}

TEST_F(ComputeSummaryTest, RttFieldsCopied) {
  stats.tmin = 10000;
  stats.tmax = 50000;
  stats.tsum = 30000;
  stats.tsum2 = 30000.0 * 30000.0;
  stats.timing = 1;
  auto s = compute();
  EXPECT_EQ(s.tmin, 10000);
  EXPECT_EQ(s.tmax, 50000);
  EXPECT_DOUBLE_EQ(s.tsum, 30000.0);
  EXPECT_DOUBLE_EQ(s.tsum2, 30000.0 * 30000.0);
  EXPECT_EQ(s.timing, 1);
}

TEST_F(ComputeSummaryTest, NullStatsZeroesOut) {
  t_ftping_summary s{};
  s.ntransmitted = 99;
  ping_stats_compute_summary(nullptr, "example.com", 1000, &s);
  EXPECT_EQ(s.ntransmitted, 0);
}

TEST_F(ComputeSummaryTest, NullOutputDoesNotCrash) {
  ping_stats_compute_summary(&stats, "example.com", 1000, nullptr);
  SUCCEED();
}
