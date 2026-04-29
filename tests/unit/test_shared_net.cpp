#include <arpa/inet.h>
#include <cstring>
#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>

extern "C" {
#include "shared/shared_error.h"
#include "shared/shared_net.h"
}

/* ──────────────────────────── dns_lookup() ──────────────────────────── */

class DnsLookupTest : public ::testing::Test {
protected:
  struct sockaddr_in addr = {};
  char ip_str[INET_ADDRSTRLEN];

  void SetUp() override {
    memset(&addr, 0, sizeof(addr));
    memset(ip_str, 0, sizeof(ip_str));
    test_err_jmp_buf_set = 1;
  }

  void TearDown() override { test_err_jmp_buf_set = 0; }
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
    SUCCEED() << "The domain does not resolve, but it should not crash.";
  }
}

TEST_F(DnsLookupTest, ResolvesValidDomain) {
  dns_lookup("example.com", &addr);

  EXPECT_EQ(addr.sin_family, AF_INET);
  EXPECT_NE(addr.sin_addr.s_addr, 0);
}

TEST_F(DnsLookupTest, FailsToResolveInvalidDomain) {
  if (setjmp(test_err_jmp_buf) == 0) {
    dns_lookup("this.host.does.not.exist.example.invalid", &addr);
    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}

TEST_F(DnsLookupTest, FailsVoidDomain) {
  if (setjmp(test_err_jmp_buf) == 0) {
    dns_lookup("", &addr);
    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}

TEST_F(DnsLookupTest, FailsNULLDomain) {
  if (setjmp(test_err_jmp_buf) == 0) {
    dns_lookup(NULL, &addr);
    FAIL() << "Expected dns_lookup to longjmp on invalid hostname";
  } else {
    EXPECT_NE(last_error_status, 0);
    EXPECT_STRNE(last_error_message, "");
    EXPECT_TRUE(strstr(last_error_message, "getaddrinfo failed") != nullptr);
  }
}

/* ──────────────────────────── is_ipv6_address() ──────────────────────────── */

class IsIpv6AddressTest : public ::testing::Test {};

TEST_F(IsIpv6AddressTest, ReturnsTrueForValidIpv6Address) {
  EXPECT_TRUE(is_ipv6_address("2001:db8::ff00:42:8329"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForInvalidIpv6Address) {
  EXPECT_FALSE(is_ipv6_address("2001:db8::ff00:42:8329:"));
}

TEST_F(IsIpv6AddressTest, ReturnsTrueForValidCompressedIpv6) {
  EXPECT_TRUE(is_ipv6_address("::1"));
  EXPECT_TRUE(is_ipv6_address("fe80::"));
  EXPECT_TRUE(is_ipv6_address("2001:db8::"));
}

TEST_F(IsIpv6AddressTest, ReturnsTrueForFullIpv6Address) {
  EXPECT_TRUE(is_ipv6_address("2001:0db8:0000:0000:0000:ff00:0042:8329"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForIpv4Address) {
  EXPECT_FALSE(is_ipv6_address("192.168.1.1"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForInvalidCharacters) {
  EXPECT_FALSE(is_ipv6_address("2001:db8::ff00:42:zzzz"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForTooManySegments) {
  EXPECT_FALSE(is_ipv6_address("2001:db8:1:2:3:4:5:6:7"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForTooFewSegments) {
  EXPECT_FALSE(is_ipv6_address("2001:db8:1:2:3"));
}

TEST_F(IsIpv6AddressTest, ReturnsFalseForEmptyString) { EXPECT_FALSE(is_ipv6_address("")); }

TEST_F(IsIpv6AddressTest, ReturnsFalseForNullPointer) { EXPECT_FALSE(is_ipv6_address(nullptr)); }

/* ──────────────────────────── get_source_address() ──────────────────────────── */

TEST(GetSourceAddressTest, ValidDevice) {
  struct sockaddr_in dest = {};
  dest.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
  struct sockaddr_in src;
  get_source_address(&src, &dest, "");
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &src.sin_addr, ip_str, INET_ADDRSTRLEN);
  std::string ip_std_str(ip_str);
  std::cout << "Source IP for device 'lo': " << ip_std_str << std::endl;
  EXPECT_STREQ(ip_str, "127.0.0.1");
}
