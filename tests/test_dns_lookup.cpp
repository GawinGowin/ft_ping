#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

extern "C" {
#include "ft_ping.h"
}

class DnsLookupTest : public ::testing::Test {
protected:
  struct sockaddr_in addr = {};
  char ip_str[INET_ADDRSTRLEN];

  void SetUp() override {
    memset(&addr, 0, sizeof(addr));
    memset(ip_str, 0, sizeof(ip_str));
    // longjmp用のフラグをセット
    test_err_jmp_buf_set = 1;
  }

  void TearDown() override {
    // longjmp用のフラグをリセット
    test_err_jmp_buf_set = 0;
  }
};

TEST_F(DnsLookupTest, ResolvesLocalhost) {
  dns_lookup("localhost", &addr);

  EXPECT_EQ(addr.sin_family, AF_INET);
  inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
  EXPECT_STREQ(ip_str, "127.0.0.1");
}

TEST_F(DnsLookupTest, ResolvesLocalhostDomain) {
  if (setjmp(test_err_jmp_buf) == 0) {
    dns_lookup("localdev.me", &addr);
    EXPECT_EQ(addr.sin_family, AF_INET);
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    EXPECT_STREQ(ip_str, "127.0.0.1");
  } else {
    // ドメインが無効な場合ここに来るはずなので、このテストケースの目的とは異なる
    SUCCEED() << "The domain does not resolve, but it should not crash.";
  }
}

TEST_F(DnsLookupTest, ResolvesValidDomain) {
  dns_lookup("example.com", &addr);

  EXPECT_EQ(addr.sin_family, AF_INET);
  EXPECT_NE(addr.sin_addr.s_addr, 0);
}

TEST_F(DnsLookupTest, FailsToResolveInvalidDomain) {
  // setjmpの戻り値を確認
  // 0: 最初の呼び出し
  // 非0: longjmpによる復帰
  if (setjmp(test_err_jmp_buf) == 0) {
    // 存在しないホスト名を指定
    dns_lookup("this.host.does.not.exist.example.invalid", &addr);

    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    // longjmpによって制御が戻ってきた場合
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}

TEST_F(DnsLookupTest, FailsVoidDomain) {
  // setjmpの戻り値を確認
  // 0: 最初の呼び出し
  // 非0: longjmpによる復帰
  if (setjmp(test_err_jmp_buf) == 0) {
    // 存在しないホスト名を指定
    dns_lookup("", &addr);

    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    // longjmpによって制御が戻ってきた場合
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}

TEST_F(DnsLookupTest, FailsNULLDomain) {
  // setjmpの戻り値を確認
  // 0: 最初の呼び出し
  // 非0: longjmpによる復帰
  if (setjmp(test_err_jmp_buf) == 0) {
    // 存在しないホスト名を指定
    dns_lookup(NULL, &addr);

    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    // longjmpによって制御が戻ってきた場合
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}