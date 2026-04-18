#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

extern "C" {
#include "shared/shared_net.h"
}

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
