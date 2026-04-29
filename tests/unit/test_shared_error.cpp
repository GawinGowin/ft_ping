#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "shared/shared_error.h"
}

class SharedErrorTest : public ::testing::Test {
protected:
  void SetUp() override {
    last_error_status = 0;
    memset(last_error_message, 0, sizeof(last_error_message));
    test_err_jmp_buf_set = 0;
  }

  void TearDown() override { test_err_jmp_buf_set = 0; }
};

/* status=0 の場合: メッセージは記録されるが longjmp は発火しない */
TEST_F(SharedErrorTest, NonFatalErrorRecordsMessageWithoutJump) {
  error(0, "warning: %s code=%d", "something", 42);

  EXPECT_EQ(last_error_status, 0);
  EXPECT_NE(strstr(last_error_message, "warning: something code=42"), nullptr);
}

/* status≠0 でも jmp_buf 未設定なら longjmp しない（テスト中に強制終了させない） */
TEST_F(SharedErrorTest, FatalErrorWithoutJmpBufDoesNotJump) {
  test_err_jmp_buf_set = 0;
  error(2, "non-jumping fatal: %d", 7);

  EXPECT_EQ(last_error_status, 2);
  EXPECT_NE(strstr(last_error_message, "non-jumping fatal: 7"), nullptr);
}

/* status≠0 + jmp_buf 設定済み: longjmp が発火し、setjmp の戻り値が status と等しい */
TEST_F(SharedErrorTest, FatalErrorJumpsWhenJmpBufIsSet) {
  test_err_jmp_buf_set = 1;
  int caught = setjmp(test_err_jmp_buf);
  if (caught == 0) {
    error(3, "fatal: %s", "boom");
    FAIL() << "Expected longjmp but execution continued";
  } else {
    EXPECT_EQ(caught, 3);
    EXPECT_EQ(last_error_status, 3);
    EXPECT_NE(strstr(last_error_message, "fatal: boom"), nullptr);
  }
}

/* メッセージは毎回ゼロクリアされる（過去のメッセージが残らない） */
TEST_F(SharedErrorTest, MessageIsResetOnEachCall) {
  error(0, "first message");
  ASSERT_NE(strstr(last_error_message, "first message"), nullptr);

  error(0, "second");
  EXPECT_NE(strstr(last_error_message, "second"), nullptr);
  EXPECT_EQ(strstr(last_error_message, "first"), nullptr);
}
