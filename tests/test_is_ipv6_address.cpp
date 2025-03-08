#include <gtest/gtest.h>

extern "C" {
#include "ft_ping.h"
}

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