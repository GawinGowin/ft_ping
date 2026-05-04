#include <climits>
#include <cstring>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

extern "C" {
#include "ping_config.h"
#include "ping_loop.h"
#include "shared/shared_error.h"
}

#include "mock_vsock.h"

/* ソケット作成可否チェック（skip 判定用） */
static bool can_create_socket() {
  t_socket_st st = {};
  int ret = ping_socket_select(&st, 0);
  if (ret >= 0) {
    close(st.fd);
    return true;
  }
  return false;
}

/* ─── A. ping_init() ─────────────────────────────────────── */

class PingInitTest : public ::testing::Test {
protected:
  t_ping_session session;

  void SetUp() override {
    memset(&session, 0, sizeof(session));
    ping_config_init(&session.config);
  }

  void TearDown() override {
    if (session.net.socket_state.fd > 0)
      close(session.net.socket_state.fd);
  }
};

/* A-1: 127.0.0.1 で fd が有効になること */
TEST_F(PingInitTest, SocketFdIsValid) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  int ret = ping_init(&session, (char *)"127.0.0.1");
  EXPECT_EQ(ret, 0);
  EXPECT_GT(session.net.socket_state.fd, 0);
}

/* A-2: DNS 解決後に whereto.sin_addr が設定されること */
TEST_F(PingInitTest, WheretoAddressIsNonzero) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  ping_init(&session, (char *)"127.0.0.1");
  EXPECT_NE(session.net.whereto.sin_addr.s_addr, 0u);
}

/* A-3: ident が非ゼロ（pid ベース）になること */
TEST_F(PingInitTest, IdentIsNonzero) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  ping_init(&session, (char *)"127.0.0.1");
  EXPECT_NE(session.net.ident, 0u);
}

/* A-4: getsockopt(IP_TTL) が config.ttl と一致すること */
TEST_F(PingInitTest, TtlMatchesConfig) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.ttl = 128;
  ping_init(&session, (char *)"127.0.0.1");
  int ttl = 0;
  socklen_t len = sizeof(ttl);
  getsockopt(session.net.socket_state.fd, IPPROTO_IP, IP_TTL, &ttl, &len);
  EXPECT_EQ(ttl, 128);
}

/* A-5: 存在しないホスト名で error() → longjmp が発火すること */
TEST_F(PingInitTest, InvalidHostnameCallsError) {
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    ping_init(&session, (char *)"invalid.host.that.does.not.exist.example");
    FAIL() << "Expected longjmp";
  } else {
    EXPECT_NE(last_error_status, 0);
  }
  test_err_jmp_buf_set = 0;
}

/* ─── B. ping_send_one() ─────────────────────────────────── */

class PingSendOneTest : public ::testing::Test {
protected:
  t_ping_session session;
  void *packet;
  size_t packet_size;

  void SetUp() override {
    memset(&session, 0, sizeof(session));
    ping_config_init(&session.config);
    packet = nullptr;
    packet_size = 0;
    if (!can_create_socket())
      return;
    ping_init(&session, (char *)"127.0.0.1");
    packet_size = session.net.socket_state.ops->packet_size(session.config.datalen);
    packet = calloc(1, packet_size);
  }

  void TearDown() override {
    free(packet);
    if (session.net.socket_state.fd > 0)
      close(session.net.socket_state.fd);
  }
};

/* B-1: interval_ms=0 の初回送信で戻り値 >= 0 */
TEST_F(PingSendOneTest, IntervalZeroReturnsNonNegative) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.interval_ms = 0;
  int ret = ping_send_one(&session, packet, packet_size, 0);
  EXPECT_GE(ret, 0);
}

/* B-2: interval_ms=1000 の初回送信で戻り値 >= 900 */
TEST_F(PingSendOneTest, IntervalThousandReturnsAtLeastNineHundred) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.interval_ms = 1000;
  int ret = ping_send_one(&session, packet, packet_size, 0);
  EXPECT_GE(ret, 900);
}

/* B-3: 送信成功後に ntransmitted が 1 増加すること */
TEST_F(PingSendOneTest, NtransmittedIncrements) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.interval_ms = 1000;
  EXPECT_EQ(session.stats.ntransmitted, 0);
  ping_send_one(&session, packet, packet_size, 0);
  EXPECT_EQ(session.stats.ntransmitted, 1);
}

/* B-4: count=1 かつ ntransmitted=1 のとき、送信せず正の待ち時間を返す
 * (iputils ping_common.c:320-321 準拠。0 を返すと do-while (next<=0) が無限ループする) */
TEST_F(PingSendOneTest, CountLimitReachedDoesNotSend) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.count = 1;
  session.config.interval_ms = 1000;
  session.stats.ntransmitted = 1;
  int ret = ping_send_one(&session, packet, packet_size, 0);
  EXPECT_GT(ret, 0);
  EXPECT_EQ(session.stats.ntransmitted, 1);
}

/* B-5: count=0（無限）では count による停止が発生しない */
TEST_F(PingSendOneTest, InfiniteCountDoesNotStop) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.count = 0;
  session.config.interval_ms = 1000;
  session.stats.ntransmitted = 999;
  /* timeval をリセットして初回送信扱いにする */
  memset(&session.timer.prev_send_time, 0, sizeof(session.timer.prev_send_time));
  int ret = ping_send_one(&session, packet, packet_size, 0);
  EXPECT_GT(ret, 0);
}

/* B-6: fd を閉じた後に ping_send_one を呼ぶと、ntransmitted を増やさず
 *      次の試行までのスケジュール時間 (>0) を返すこと。
 *      iputils pinger (ping_common.c:438) と同じく、送信失敗時も負値ではなく
 *      SCHINT(interval) を返して do-while (next <= 0) で永久ループしない設計。 */
TEST_F(PingSendOneTest, ClosedFdReschedulesWithoutIncrement) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  session.config.interval_ms = 1000;
  close(session.net.socket_state.fd);
  session.net.socket_state.fd = -1;
  int before = session.stats.ntransmitted;
  int ret = ping_send_one(&session, packet, packet_size, 0);
  EXPECT_GT(ret, 0);
  EXPECT_EQ(session.stats.ntransmitted, before);
}

/* ─── C. ping_receive_replies() ────────────────────────────── */

class PingReceiveRepliesTest : public ::testing::Test {
protected:
  t_ping_session session;
  struct iovec iov;
  struct msghdr msg;
  t_ping_receive received;
  void *recv_buf;
  size_t packet_size;
  int polling;

  void SetUp() override {
    memset(&session, 0, sizeof(session));
    memset(&iov, 0, sizeof(iov));
    memset(&msg, 0, sizeof(msg));
    memset(&received, 0, sizeof(received));
    recv_buf = nullptr;
    packet_size = 0;
    polling = MSG_DONTWAIT;
    ping_config_init(&session.config);
    if (!can_create_socket())
      return;
    ping_init(&session, (char *)"127.0.0.1");
    packet_size = session.net.socket_state.ops->packet_size(session.config.datalen);
    recv_buf = calloc(1, packet_size);
    received.socket_fd = &session.net.socket_state.fd;
    received.packlen = packet_size;
    received.polling = &polling;
    received.iov = &iov;
    received.msg = &msg;
    received.iov->iov_base = recv_buf;
  }

  void TearDown() override {
    free(recv_buf);
    if (session.net.socket_state.fd > 0)
      close(session.net.socket_state.fd);
  }
};

/* C-1: データなしの MSG_DONTWAIT 即時呼び出しで 0 が返ること */
TEST_F(PingReceiveRepliesTest, NoDataReturnsZero) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  int ret = ping_receive_replies(&session, &received);
  EXPECT_EQ(ret, 0);
}

/* C-2: received が NULL のとき 0 が返ること */
TEST_F(PingReceiveRepliesTest, NullReceivedReturnsZero) {
  int ret = ping_receive_replies(&session, nullptr);
  EXPECT_EQ(ret, 0);
}

/* ─── D. ping_run() ──────────────────────────────────────── */

class PingRunTest : public ::testing::Test {
protected:
  t_ping_session session;

  void SetUp() override {
    memset(&session, 0, sizeof(session));
    ping_config_init(&session.config);
  }

  void TearDown() override {
    if (session.net.socket_state.fd > 0)
      close(session.net.socket_state.fd);
  }
};

/* D-3: is_exiting=1 のとき ping_run が即座に戻ること */
TEST_F(PingRunTest, IsExitingExitsImmediately) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  ping_init(&session, (char *)"127.0.0.1");
  session.config.interval_ms = 1000;
  session.is_exiting = 1;
  ping_run(&session);
  /* is_exiting で即座に break するのでパケットを送信しない */
  EXPECT_EQ(session.stats.ntransmitted, 0);
}

/* D-4: count=0（無限）でも is_exiting をセットすれば終了すること */
TEST_F(PingRunTest, InfiniteCountExitsOnIsExiting) {
  if (!can_create_socket())
    GTEST_SKIP() << "socket not available";
  ping_init(&session, (char *)"127.0.0.1");
  session.config.count = 0;
  session.config.interval_ms = 1000;
  session.is_exiting = 1;
  ping_run(&session);
  EXPECT_EQ(session.stats.ntransmitted, 0);
}

/* ─── E. should_use_fast_path() ─────────────────────────── */

#ifdef TESTING

class ShouldUseFastPathTest : public ::testing::Test {
protected:
  t_ping_config config;

  void SetUp() override {
    ping_config_init(&config);
    config.opt_adaptive = 0;
    config.opt_flood_poll = 0;
  }
};

/* E-1: opt_adaptive=1 → next の値に関係なく 1 */
TEST_F(ShouldUseFastPathTest, AdaptiveModeAlwaysTrue) {
  config.opt_adaptive = 1;
  EXPECT_EQ(should_use_fast_path_test(&config, 0), 1);
  EXPECT_EQ(should_use_fast_path_test(&config, 10000), 1);
}

/* E-2: opt_flood_poll=1 → 常に 1 */
TEST_F(ShouldUseFastPathTest, FloodPollAlwaysTrue) {
  config.opt_flood_poll = 1;
  EXPECT_EQ(should_use_fast_path_test(&config, 0), 1);
  EXPECT_EQ(should_use_fast_path_test(&config, 10000), 1);
}

/* E-3: next < SCHINT(interval_ms) → 1  (interval_ms=0 → SCHINT=10, next=9) */
TEST_F(ShouldUseFastPathTest, NextLessThanSchintReturnsTrue) {
  config.interval_ms = 0; /* SCHINT(0) == 10 */
  EXPECT_EQ(should_use_fast_path_test(&config, 9), 1);
}

/* E-4: adaptive=0, flood_poll=0, next >= SCHINT(interval_ms) → 0 */
TEST_F(ShouldUseFastPathTest, NextGeSchintReturnsFalse) {
  config.interval_ms = 0; /* SCHINT(0) == 10 */
  EXPECT_EQ(should_use_fast_path_test(&config, 10), 0);
  EXPECT_EQ(should_use_fast_path_test(&config, 1000), 0);
}

/* E-5: interval_ms=1000 → SCHINT=1000, next=999 → 1 */
TEST_F(ShouldUseFastPathTest, LargeIntervalNextLessThanSchint) {
  config.interval_ms = 1000;
  EXPECT_EQ(should_use_fast_path_test(&config, 999), 1);
}

/* E-6: interval_ms=1000 → SCHINT=1000, next=1000 → 0 */
TEST_F(ShouldUseFastPathTest, LargeIntervalNextGeSchint) {
  config.interval_ms = 1000;
  EXPECT_EQ(should_use_fast_path_test(&config, 1000), 0);
}

#endif /* TESTING */

/* ─── F. Mock vtable: 実ソケット不要のテスト ─────────────────────────── */

class PingSendOneMockTest : public ::testing::Test {
protected:
  t_ping_session session;
  std::vector<char> packet_buf;

  void SetUp() override {
    mock_vsock_reset();
    memset(&session, 0, sizeof(session));
    ping_config_init(&session.config);
    /* 擬似ソケット（fd=999, ops=&Mock_socket_ops）をセット。
     * sendto() は fd=999 で必ず失敗するが、build_ipheader は呼ばれる。 */
    mock_vsock_attach(&session.net.socket_state);
    packet_buf.assign(64, 0);
  }
};

/* F-1: ping_send_one() を呼ぶと Mock_socket_ops.build_ipheader が起動する */
TEST_F(PingSendOneMockTest, BuildIpHeaderIsInvoked) {
  ASSERT_EQ(g_mock_vsock_state.build_ipheader_calls, 0);

  /* fd=999 で send_packet は失敗するので戻り値は問わない。
   * build_ipheader が呼ばれたことだけ検証する。 */
  ping_send_one(&session, packet_buf.data(), packet_buf.size(), 0);

  EXPECT_EQ(g_mock_vsock_state.build_ipheader_calls, 1);
  EXPECT_EQ(g_mock_vsock_state.last_seq, 0u);
  EXPECT_EQ(g_mock_vsock_state.last_datalen,
            static_cast<size_t>(session.config.datalen));
}

/* F-2: count に達していたら build_ipheader は呼ばれない */
TEST_F(PingSendOneMockTest, BuildIpHeaderSkippedWhenCountReached) {
  session.config.count = 3;
  session.stats.ntransmitted = 3;

  ping_send_one(&session, packet_buf.data(), packet_buf.size(), 0);

  EXPECT_EQ(g_mock_vsock_state.build_ipheader_calls, 0);
}
