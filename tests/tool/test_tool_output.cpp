#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <string>

extern "C" {
#include "tool_output.h"
}

/* stdout を一時パイプ経由で文字列として捕捉するヘルパー。
 * gtest の CaptureStdout を使う。 */
static std::string capture(std::function<void()> fn) {
  testing::internal::CaptureStdout();
  fn();
  return testing::internal::GetCapturedStdout();
}

/* ─── tool_output_header ─────────────────────────────── */

class HeaderTest : public ::testing::Test {
protected:
  t_ping_config config{};
  void SetUp() override {
    config.hostname = "localhost";
    config.datalen = 56;
  }
};

TEST_F(HeaderTest, BasicFormat) {
  struct in_addr addr;
  inet_aton("127.0.0.1", &addr);
  std::string out = capture([&]() { tool_output_header(&config, addr, 84); });
  EXPECT_EQ(out, "PING localhost (127.0.0.1) 56(84) bytes of data.\n");
}

TEST_F(HeaderTest, NullConfigDoesNotCrash) {
  struct in_addr addr{};
  std::string out = capture([&]() { tool_output_header(nullptr, addr, 0); });
  EXPECT_EQ(out, "");
}

/* ─── tool_output_reply ──────────────────────────────── */

class ReplyTest : public ::testing::Test {
protected:
  t_ping_config config{};
  t_ftping_reply reply{};
  void SetUp() override {
    config.hostname = "localhost";
    config.datalen = 56;
    config.opt_verbose = 0;
    config.opt_ptimeofday = 0;

    inet_aton("127.0.0.1", &reply.from_addr);
    reply.bytes = 64;
    reply.seq = 1;
    reply.triptime_us = 27; /* 0.027 ms */
    reply.is_duplicate = 0;
    reply.recv_time.tv_sec = 0;
    reply.recv_time.tv_usec = 0;
  }
};

TEST_F(ReplyTest, BasicReply) {
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_EQ(out, "64 bytes from 127.0.0.1: icmp_seq=1 time=0.027 ms\n");
}

TEST_F(ReplyTest, NoTimingOmitsTimeField) {
  reply.triptime_us = -1;
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_EQ(out, "64 bytes from 127.0.0.1: icmp_seq=1\n");
}

TEST_F(ReplyTest, DuplicateWithVerbosePrintsDup) {
  config.opt_verbose = 1;
  reply.is_duplicate = 1;
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_NE(out.find("(DUP!)"), std::string::npos);
}

TEST_F(ReplyTest, DuplicateWithoutVerboseSilent) {
  config.opt_verbose = 0;
  reply.is_duplicate = 1;
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_EQ(out, "");
}

TEST_F(ReplyTest, PtimeofdayPrefix) {
  config.opt_ptimeofday = 1;
  reply.recv_time.tv_sec = 1234567890;
  reply.recv_time.tv_usec = 123;
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_EQ(out.rfind("[1234567890.000123] ", 0), 0u);
}

TEST_F(ReplyTest, NullReplyDoesNotCrash) {
  std::string out = capture([&]() { tool_output_reply(nullptr, &config); });
  EXPECT_EQ(out, "");
}

TEST_F(ReplyTest, NullCtxAllowed) {
  /* ctx == NULL でも基本フォーマットは出る */
  std::string out = capture([&]() { tool_output_reply(&reply, nullptr); });
  EXPECT_EQ(out, "64 bytes from 127.0.0.1: icmp_seq=1 time=0.027 ms\n");
}

/* ─── tool_output_finish ─────────────────────────────── */

class FinishTest : public ::testing::Test {
protected:
  t_ftping_summary s{};
  void SetUp() override {
    s.hostname = "localhost";
    s.interval_ms = 1000;
  }
};

TEST_F(FinishTest, HeaderHasHostname) {
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("--- localhost ping statistics ---"), std::string::npos);
}

TEST_F(FinishTest, BasicCounts) {
  s.ntransmitted = 2;
  s.nreceived = 2;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("2 packets transmitted, 2 received"), std::string::npos);
  EXPECT_NE(out.find("0.0% packet loss"), std::string::npos);
  EXPECT_NE(out.find("time 2000ms"), std::string::npos);
}

TEST_F(FinishTest, FiftyPercentLoss) {
  s.ntransmitted = 4;
  s.nreceived = 2;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("50.0% packet loss"), std::string::npos);
}

TEST_F(FinishTest, DuplicatesShown) {
  s.ntransmitted = 2;
  s.nreceived = 2;
  s.nrepeats = 1;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("+1 duplicates"), std::string::npos);
}

TEST_F(FinishTest, NoDuplicatesWhenZero) {
  s.ntransmitted = 1;
  s.nreceived = 1;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_EQ(out.find("duplicates"), std::string::npos);
}

TEST_F(FinishTest, CorruptedAndErrorsShown) {
  s.ntransmitted = 3;
  s.nreceived = 2;
  s.nchecksum = 1;
  s.nerrors = 5;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("+1 corrupted"), std::string::npos);
  EXPECT_NE(out.find("+5 errors"), std::string::npos);
}

TEST_F(FinishTest, RttLineWhenTimingAndReceived) {
  s.ntransmitted = 1;
  s.nreceived = 1;
  s.timing = 1;
  s.tmin = 10000; /* 10ms */
  s.tmax = 10000;
  s.tsum = 10000;
  s.tsum2 = 10000.0 * 10000.0;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("rtt min/avg/max/mdev = 10.000/10.000/10.000/0.000 ms"), std::string::npos);
}

TEST_F(FinishTest, RttLineAbsentWhenTimingOff) {
  s.ntransmitted = 1;
  s.nreceived = 1;
  s.timing = 0;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_EQ(out.find("rtt"), std::string::npos);
}

TEST_F(FinishTest, RttLineAbsentWhenNoReceived) {
  s.ntransmitted = 3;
  s.nreceived = 0;
  s.timing = 1;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_EQ(out.find("rtt"), std::string::npos);
}

TEST_F(FinishTest, NullSummaryDoesNotCrash) {
  std::string out = capture([&]() { tool_output_finish(nullptr); });
  EXPECT_EQ(out, "");
}
