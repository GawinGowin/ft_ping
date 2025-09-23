#include <gtest/gtest.h>

extern "C" {
#include "ft_ping.h"
}

class CreateSocketTest : public ::testing::Test {};

TEST_F(CreateSocketTest, CreatesRawSocket) {
  t_socket_st socket_state;
  int ret = create_socket(&socket_state);
  EXPECT_NE(ret, -1);
  EXPECT_TRUE(socket_state.socktype == SOCK_RAW || socket_state.socktype == SOCK_DGRAM);
  EXPECT_GE(socket_state.fd, 0);
  close(socket_state.fd);
}
