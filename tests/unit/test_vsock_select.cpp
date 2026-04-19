#include <errno.h>
#include <gtest/gtest.h>

extern "C" {
#include "vsock/vsock.h"
}

class VsockSelectTest : public ::testing::Test {};

// ping_socket_select が成功したとき、fd が有効で ops が設定されること
TEST_F(VsockSelectTest, SelectSetsValidFdAndOps) {
  t_socket_st state = {};
  int ret = ping_socket_select(&state);

  if (ret < 0) {
    GTEST_SKIP() << "socket creation requires privilege or is unavailable";
  }

  EXPECT_GE(state.fd, 0);
  EXPECT_NE(state.ops, nullptr);
  EXPECT_TRUE(state.ops == &Ping_socket_raw_ops || state.ops == &Ping_socket_dgram_ops);
  close(state.fd);
}

// SOCK_RAW が取れたとき ops は raw_ops を指すこと
// 実行方法: sudo [テスト実行ファイル名] --gtest_filter="VsockSelectTest.RawSocketGetsRawOps"
TEST_F(VsockSelectTest, RawSocketGetsRawOps) {
  t_socket_st state = {};
  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    GTEST_SKIP() << "SOCK_RAW not available (no privilege)";
  }
  close(fd);

  ping_socket_select(&state);
  EXPECT_EQ(state.ops, &Ping_socket_raw_ops);
  close(state.fd);
}

// SOCK_DGRAM が選ばれたとき ops は dgram_ops を指すこと
// (SOCK_RAW が取れる環境では実行不可なのでスキップ)
TEST_F(VsockSelectTest, DgramSocketGetsDgramOps) {
  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd >= 0) {
    close(fd);
    GTEST_SKIP() << "SOCK_RAW available; DGRAM fallback path not exercised";
  }
  if (errno != EPERM && errno != EACCES) {
    GTEST_SKIP() << "unexpected errno, skip";
  }

  t_socket_st state = {};
  int ret = ping_socket_select(&state);
  if (ret < 0) {
    GTEST_SKIP() << "neither RAW nor DGRAM socket available";
  }

  EXPECT_EQ(state.ops, &Ping_socket_dgram_ops);
  close(state.fd);
}

// 失敗時に fd と socktype が -1 になること
// (通常環境では再現困難なので ops ポインタの一貫性を確認)
TEST_F(VsockSelectTest, OpsMatchSocktype) {
  t_socket_st state = {};
  int ret = ping_socket_select(&state);
  if (ret < 0) {
    GTEST_SKIP() << "socket not available";
  }

  EXPECT_TRUE(state.ops == &Ping_socket_raw_ops || state.ops == &Ping_socket_dgram_ops);
  close(state.fd);
}
