#include <cstring>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>

extern "C" {
#include "ft_ping.h"
}

// dns_lookup関数が正常に動作するかテストする
TEST(DnsLookupTest, ResolvesLocalhost) {
  struct sockaddr_in addr = {};

  // localhostを解決する
  dns_lookup("localhost", &addr);

  // localhostは通常127.0.0.1に解決される
  EXPECT_EQ(addr.sin_family, AF_INET);
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
  EXPECT_STREQ(ip_str, "127.0.0.1");
}

// 実在するドメイン名を解決できるかテストする
TEST(DnsLookupTest, ResolvesValidDomain) {
  struct sockaddr_in addr = {};

  // example.comを解決する
  dns_lookup("example.com", &addr);

  // 解決されたIPアドレスが空でないこと
  EXPECT_EQ(addr.sin_family, AF_INET);
  EXPECT_NE(addr.sin_addr.s_addr, 0);
}
