#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

extern "C" {
#include "ft_ping.h"
}

namespace {
std::string captured_error_message;
int captured_exit_status = 0;
bool exit_called = false;
} // namespace

void errorWrapper(int status, const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  captured_error_message = buffer;
  captured_exit_status = status;
  exit_called = true;

  if (status) {
    throw std::runtime_error("exit called with status: " + std::to_string(status));
  }
}

class ParseLongTest : public ::testing::Test {
protected:
  void SetUp() override {
    captured_error_message.clear();
    captured_exit_status = 0;
    exit_called = false;
    program_invocation_short_name = const_cast<char *>("ft_ping_test");
  }
};

TEST_F(ParseLongTest, ValidInput) {
  const char *input = "123";
  const char *msg = "test message";
  const long min = 0;
  const long max = 1000;

  long result = parse_long(input, msg, min, max, errorWrapper);
  EXPECT_EQ(result, 123);
  EXPECT_FALSE(exit_called);
}

TEST_F(ParseLongTest, BoundaryValues) {
  EXPECT_EQ(parse_long("0", "test", 0, 1000, errorWrapper), 0);
  EXPECT_FALSE(exit_called);

  EXPECT_EQ(parse_long("1000", "test", 0, 1000, errorWrapper), 1000);
  EXPECT_FALSE(exit_called);
}

TEST_F(ParseLongTest, NullOrEmptyInput) {
  const char *msg = "test message";
  const long min = 0;
  const long max = 1000;

  EXPECT_THROW(parse_long(NULL, msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
  EXPECT_EQ(captured_exit_status, 1);
  EXPECT_TRUE(captured_error_message.find("test message: ") != std::string::npos);

  exit_called = false;
  captured_error_message.clear();

  EXPECT_THROW(parse_long("", msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
  EXPECT_EQ(captured_exit_status, 1);
}

TEST_F(ParseLongTest, InvalidInput) {
  const char *msg = "test message";
  const long min = 0;
  const long max = 1000;

  EXPECT_THROW(parse_long("123abc", msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
  EXPECT_EQ(captured_exit_status, 1);

  exit_called = false;
  captured_error_message.clear();

  EXPECT_THROW(parse_long("abc", msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
}

TEST_F(ParseLongTest, OutOfRangeValues) {
  const char *msg = "test message";
  const long min = 10;
  const long max = 100;

  EXPECT_THROW(parse_long("5", msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
  EXPECT_EQ(captured_exit_status, 1);
  EXPECT_TRUE(captured_error_message.find("out of range") != std::string::npos);

  exit_called = false;
  captured_error_message.clear();

  EXPECT_THROW(parse_long("200", msg, min, max, errorWrapper), std::runtime_error);
  EXPECT_TRUE(exit_called);
  EXPECT_EQ(captured_exit_status, 1);
  EXPECT_TRUE(captured_error_message.find("out of range") != std::string::npos);
}

char *program_invocation_short_name;
