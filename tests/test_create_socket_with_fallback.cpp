#include <gtest/gtest.h>

extern "C" {
#include "ft_ping.h"
}

class CreateSocketTest : public ::testing::Test {};

TEST_F(CreateSocketTest, CreatesRawSocket) {
  int sockfd = create_socket_with_fallback();

  EXPECT_GE(sockfd, 0);
  close(sockfd);
}
