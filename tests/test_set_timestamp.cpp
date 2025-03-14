#include <gtest/gtest.h>

extern "C" {
#include "icmp.h"
}

TEST(SetTimeStampTEST, SetTimeStamp) {
  const size_t datasize = 64;
  void *packet = malloc(sizeof(t_icmp) + datasize);
  ASSERT_NE(packet, nullptr);

  struct timeval timestamp;
  gettimeofday(&timestamp, NULL);
  set_timestamp(packet, datasize, &timestamp);
  t_icmp *icmp = (t_icmp *)packet;
  ASSERT_NE(icmp, nullptr);
  EXPECT_NE(icmp->timestamp, 0);
  free(packet);
}
