#include <gtest/gtest.h>

extern "C" {
#include "icmp.h"
}

struct TestData {
  uint16_t id;
  uint16_t seq;
  t_icmp expected_icmp;
};

bool operator==(const t_icmp &lhs, const t_icmp &rhs) {
  return lhs.type == rhs.type && lhs.code == rhs.code && lhs.id == rhs.id && lhs.seq == rhs.seq;
}

std::ostream &operator<<(std::ostream &os, const TestData &data) {
  os << "id: " << (int)data.id << ", "
     << "seq: " << (int)data.seq;
  return os;
}

class CreateEchoRequestPacket : public ::testing::TestWithParam<TestData> {};

TEST_P(CreateEchoRequestPacket, Case) {
  t_icmp icmp;

  auto param = GetParam();

  create_echo_request_packet(&icmp, param.id, param.seq);
  EXPECT_EQ(icmp, param.expected_icmp);
}

const TestData RequestTestData[] = {
    {
        .id = 1,
        .seq = 1,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(1),
                .seq = htons(1),
                .timestamp = 0,
            },
    },
    {
        .id = 2,
        .seq = 2,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(2),
                .seq = htons(2),
                .timestamp = 0,
            },
    },
    {
        .id = 3,
        .seq = 3,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(3),
                .seq = htons(3),
                .timestamp = 0,
            },
    },
    {
        .id = 4,
        .seq = 4,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(4),
                .seq = htons(4),
                .timestamp = 0,
            },
    },
    {
        .id = 5,
        .seq = 5,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(5),
                .seq = htons(5),
                .timestamp = 0,
            },
    },
    {
        .id = 6,
        .seq = 6,
        .expected_icmp =
            {
                .type = ICMP_ECHO,
                .code = 0,
                .checksum = 0,
                .id = htons(6),
                .seq = htons(6),
                .timestamp = 0,
            },
    },
    {
        .id = 7,
        .seq = 7,
        .expected_icmp =
            {.type = ICMP_ECHO,
             .code = 0,
             .checksum = 0,
             .id = htons(7),
             .seq = htons(7),
             .timestamp = 0},
    },
};

INSTANTIATE_TEST_SUITE_P(PacketTest, CreateEchoRequestPacket, ::testing::ValuesIn(RequestTestData));