#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <gtest/gtest.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

extern "C" {
#include "ft_ping.h"
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