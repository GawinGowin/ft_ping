#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/ip_icmp.h>
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
  std::string out = capture([&]() { tool_output_header(&config, addr, 0); });
  EXPECT_EQ(out, "PING localhost (127.0.0.1): 56 data bytes\n");
}

TEST_F(HeaderTest, NullConfigDoesNotCrash) {
  struct in_addr addr{};
  std::string out = capture([&]() { tool_output_header(nullptr, addr, 100); });
  EXPECT_EQ(out, "");
}

class HeaderTestVerbose : public ::testing::Test {
protected:
  t_ping_config config{};
  void SetUp() override {
    config.hostname = "localhost";
    config.datalen = 56;
    config.opt_verbose = 1;
  }
};

TEST_F(HeaderTestVerbose, BasicFormat) {
  struct in_addr addr;
  inet_aton("127.0.0.1", &addr);
  uint16_t ident = 0xabcd;
  std::string out = capture([&]() { tool_output_header(&config, addr, ident); });
  EXPECT_EQ(out, "PING localhost (127.0.0.1): 56 data bytes, id 0xabcd = 43981\n");
}

TEST_F(HeaderTestVerbose, NullHostname) {
  struct in_addr addr;
  inet_aton("8.8.8.8", &addr);
  config.hostname = nullptr;
  uint16_t ident = 100;
  std::string out = capture([&]() { tool_output_header(&config, addr, ident); });
  EXPECT_EQ(out, "PING  (8.8.8.8): 56 data bytes, id 0x64 = 100\n");
}

/* ─── tool_output_reply ──────────────────────────────── */

class ReplyTest : public ::testing::Test {
protected:
  t_ping_config config{};
  t_ftping_reply reply{};
  void SetUp() override {
    config.hostname = "localhost";
    config.datalen = 56;
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

TEST_F(ReplyTest, DuplicatePrintsDup) {
  reply.is_duplicate = 1;
  std::string out = capture([&]() { tool_output_reply(&reply, &config); });
  EXPECT_NE(out.find("(DUP!)"), std::string::npos);
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
  EXPECT_NE(out.find("2 packets transmitted, 2 packets received"), std::string::npos);
  EXPECT_NE(out.find("0% packet loss"), std::string::npos);
  EXPECT_EQ(out.find("time "), std::string::npos);
}

TEST_F(FinishTest, FiftyPercentLoss) {
  s.ntransmitted = 4;
  s.nreceived = 2;
  std::string out = capture([&]() { tool_output_finish(&s); });
  EXPECT_NE(out.find("50% packet loss"), std::string::npos);
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
  EXPECT_NE(out.find("round-trip min/avg/max/stddev = 10.000/10.000/10.000/0.000 ms"), std::string::npos);
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

/* ─── tool_output_error ──────────────────────────────── */

class ErrorTest : public ::testing::Test {
protected:
  t_ping_config config{};
  t_ftping_error_event ev{};
  void SetUp() override {
    config.opt_verbose = 1;
    inet_aton("10.0.0.1", &ev.from_addr);
    ev.orig_seq = 3;
    ev.orig_seq_valid = 1;
  }
};

TEST_F(ErrorTest, TimeExceededWithSeq) {
  ev.icmp_type = ICMP_TIME_EXCEEDED;
  std::string out = capture([&]() { tool_output_error(&ev, &config); });
  EXPECT_EQ(out, "From 10.0.0.1: icmp_seq=3 Time exceeded\n");
}

TEST_F(ErrorTest, DestUnreachWithoutSeq) {
  ev.icmp_type = ICMP_DEST_UNREACH;
  ev.orig_seq_valid = 0;
  std::string out = capture([&]() { tool_output_error(&ev, &config); });
  EXPECT_EQ(out, "From 10.0.0.1: Destination Host Unreachable\n");
}

TEST_F(ErrorTest, SuppressedWhenVerboseOff) {
  config.opt_verbose = 0;
  ev.icmp_type = ICMP_TIME_EXCEEDED;
  std::string out = capture([&]() { tool_output_error(&ev, &config); });
  EXPECT_EQ(out, "");
}

TEST_F(ErrorTest, NullEvDoesNotCrash) {
  std::string out = capture([&]() { tool_output_error(nullptr, &config); });
  EXPECT_EQ(out, "");
}

TEST_F(ErrorTest, NullCtxSuppressesOutput) {
  ev.icmp_type = ICMP_TIME_EXCEEDED;
  std::string out = capture([&]() { tool_output_error(&ev, nullptr); });
  EXPECT_EQ(out, "");
}
