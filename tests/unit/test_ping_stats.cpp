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

/* ─── ping_stats_finish ─────────────────────────────────── */

class FinishTest : public ::testing::Test {
protected:
  t_ping_stats_internal stats = {};

  std::string capture_finish(int interval_ms = 1000) {
    testing::internal::CaptureStdout();
    ping_stats_finish(&stats, "example.com", interval_ms);
    return testing::internal::GetCapturedStdout();
  }
};

TEST_F(FinishTest, HeaderContainsHostname) {
  std::string out = capture_finish();
  EXPECT_NE(out.find("example.com ping statistics"), std::string::npos);
}

TEST_F(FinishTest, ZeroPackets) {
  std::string out = capture_finish();
  EXPECT_NE(out.find("0 packets transmitted, 0 received"), std::string::npos);
}

TEST_F(FinishTest, PacketCountsAppear) {
  stats.ntransmitted = 5;
  stats.nreceived = 4;
  std::string out = capture_finish();
  EXPECT_NE(out.find("5 packets transmitted, 4 received"), std::string::npos);
}

TEST_F(FinishTest, ZeroPercentLoss) {
  stats.ntransmitted = 4;
  stats.nreceived = 4;
  std::string out = capture_finish();
  EXPECT_NE(out.find("0.0% packet loss"), std::string::npos);
}

TEST_F(FinishTest, HundredPercentLoss) {
  stats.ntransmitted = 4;
  stats.nreceived = 0;
  std::string out = capture_finish();
  EXPECT_NE(out.find("100.0% packet loss"), std::string::npos);
}

TEST_F(FinishTest, FiftyPercentLoss) {
  stats.ntransmitted = 4;
  stats.nreceived = 2;
  std::string out = capture_finish();
  EXPECT_NE(out.find("50.0% packet loss"), std::string::npos);
}

TEST_F(FinishTest, DuplicatesAppear) {
  stats.ntransmitted = 3;
  stats.nreceived = 3;
  stats.nrepeats = 2;
  std::string out = capture_finish();
  EXPECT_NE(out.find("+2 duplicates"), std::string::npos);
}

TEST_F(FinishTest, NoDuplicatesLineWhenZero) {
  stats.ntransmitted = 1;
  stats.nreceived = 1;
  std::string out = capture_finish();
  EXPECT_EQ(out.find("duplicates"), std::string::npos);
}

TEST_F(FinishTest, CorruptedAppears) {
  stats.ntransmitted = 3;
  stats.nreceived = 2;
  stats.nchecksum = 1;
  std::string out = capture_finish();
  EXPECT_NE(out.find("+1 corrupted"), std::string::npos);
}

TEST_F(FinishTest, ErrorsAppear) {
  stats.ntransmitted = 3;
  stats.nreceived = 2;
  stats.nerrors = 1;
  std::string out = capture_finish();
  EXPECT_NE(out.find("+1 errors"), std::string::npos);
}

TEST_F(FinishTest, TimeLineUsesIntervalMs) {
  stats.ntransmitted = 3;
  std::string out = capture_finish(/*interval_ms=*/200);
  EXPECT_NE(out.find("time 600ms"), std::string::npos);
}

TEST_F(FinishTest, RttLineAppearsWhenReceivedAndTiming) {
  stats.ntransmitted = 1;
  stats.nreceived = 1;
  stats.timing = 1;
  stats.tmin = 10000;
  stats.tmax = 10000;
  stats.tsum = 10000;
  stats.tsum2 = 10000.0 * 10000.0;
  std::string out = capture_finish();
  EXPECT_NE(out.find("rtt min/avg/max/mdev"), std::string::npos);
}

TEST_F(FinishTest, RttLineAbsentWhenTimingOff) {
  stats.ntransmitted = 1;
  stats.nreceived = 1;
  stats.timing = 0;
  std::string out = capture_finish();
  EXPECT_EQ(out.find("rtt"), std::string::npos);
}

TEST_F(FinishTest, RttLineAbsentWhenNoReceived) {
  stats.ntransmitted = 3;
  stats.nreceived = 0;
  stats.timing = 1;
  std::string out = capture_finish();
  EXPECT_EQ(out.find("rtt"), std::string::npos);
}

TEST_F(FinishTest, RttValuesCorrect) {
  /* 1回だけ受信、RTT=10ms(=10000us) → min=avg=max=10.000, mdev=0.000 */
  stats.ntransmitted = 1;
  stats.nreceived = 1;
  stats.timing = 1;
  stats.tmin = 10000;
  stats.tmax = 10000;
  stats.tsum = 10000;
  stats.tsum2 = 10000.0 * 10000.0;
  std::string out = capture_finish();
  EXPECT_NE(out.find("10.000/10.000/10.000/0.000 ms"), std::string::npos);
}
