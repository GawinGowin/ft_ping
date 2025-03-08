#include <gtest/gtest.h>

extern "C" {
#include "icmp.h"
}

TEST(GeneratePacketData, Normal) {
  unsigned char packet[1024 + 8]; // 先頭8ビットずらしたところからDataが始まる
  generate_packet_data(packet, 1024);
  unsigned char *data = GET_PACKET_DATA(packet);
  for (int i = 0; i < 1024; i++) {
    ASSERT_EQ(data[i], (unsigned char)i % UCHAR_MAX);
  }
}
