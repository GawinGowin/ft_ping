#include <arpa/inet.h>
#include <climits>
#include <cstring>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <vector>

extern "C" {
#include "ping_config.h"
#include "shared/shared_error.h"
#include "tool_getparam.h"
}

class ToolParseArgsTest : public ::testing::Test {
protected:
  t_ping_config config;
  std::vector<char *> argv_storage;

  void SetUp() override {
    ping_config_init(&config);
    optind = 1;
    opterr = 0;
    test_err_jmp_buf_set = 0;
    last_error_status = 0;
    memset(last_error_message, 0, sizeof(last_error_message));
  }

  void TearDown() override {
    for (auto p : argv_storage)
      free(p);
    argv_storage.clear();
    test_err_jmp_buf_set = 0;
  }

  /* {"ft_ping", "-t", "64", "host"} → argc=4, argv=char**  */
  void make_argv(std::initializer_list<const char *> args, int *argc, char ***argv) {
    argv_storage.clear();
    for (auto s : args)
      argv_storage.push_back(strdup(s));
    *argc = (int)argv_storage.size();
    *argv = argv_storage.data();
  }
};

/* ─── A. ブールフラグ ─────────────────────────────────────── */

TEST_F(ToolParseArgsTest, AdaptiveFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-A", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.opt_adaptive, 1u);
}

TEST_F(ToolParseArgsTest, VerboseFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-v", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.opt_verbose, 1u);
}

TEST_F(ToolParseArgsTest, PrintTimestampFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-D", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.opt_ptimeofday, 1u);
}

/* ─── B. 数値フラグ（正常値）──────────────────────────────── */

TEST_F(ToolParseArgsTest, TtlFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "128", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ttl, 128); /* デフォルト 64 と異なる値で確認 */
}

TEST_F(ToolParseArgsTest, TosFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-Q", "16", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.tos, 16);
}

TEST_F(ToolParseArgsTest, CountFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-c", "5", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.count, 5);
}

TEST_F(ToolParseArgsTest, IdentFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-e", "1234", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ident, 1234);
}

TEST_F(ToolParseArgsTest, SndbufFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-S", "65536", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.sndbuf, 65536);
}

TEST_F(ToolParseArgsTest, DatalenFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-s", "128", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.datalen, 128);
}

TEST_F(ToolParseArgsTest, PreloadFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-l", "3", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.preload, 3);
}

TEST_F(ToolParseArgsTest, DeadlineFlag) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-w", "10", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.deadline_sec, 10);
}

/* ─── C. 境界値 ───────────────────────────────────────────── */

TEST_F(ToolParseArgsTest, TtlMin) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "1", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ttl, 1);
}

TEST_F(ToolParseArgsTest, TtlMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "255", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ttl, 255);
}

TEST_F(ToolParseArgsTest, TosMin) {
  int argc;
  char **argv;
  config.tos = 100; /* デフォルト 0 と異なる値にしてから上書きを確認 */
  make_argv({"ft_ping", "-Q", "0", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.tos, 0);
}

TEST_F(ToolParseArgsTest, TosMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-Q", "255", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.tos, 255);
}

TEST_F(ToolParseArgsTest, CountZeroMeansInfinite) {
  int argc;
  char **argv;
  config.count = 5; /* デフォルト 0 と異なる値にしてから上書きを確認 */
  make_argv({"ft_ping", "-c", "0", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.count, 0);
}

TEST_F(ToolParseArgsTest, IdentZero) {
  int argc;
  char **argv;
  config.ident = htons(999); /* デフォルト 0 と異なる値にしてから上書きを確認 */
  make_argv({"ft_ping", "-e", "0", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ident, (uint16_t)htons(0));
}

TEST_F(ToolParseArgsTest, IdentMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-e", "65535", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.ident, htons(65535));
}

TEST_F(ToolParseArgsTest, PreloadZero) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-l", "0", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.preload, 0);
}

TEST_F(ToolParseArgsTest, PreloadMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-l", "65536", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.preload, 65536);
}

TEST_F(ToolParseArgsTest, DatalenZero) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-s", "0", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.datalen, 0);
}

/* ─── D. エラーケース（範囲外・不正値）──────────────────── */

TEST_F(ToolParseArgsTest, TtlBelowMin) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "0", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

TEST_F(ToolParseArgsTest, TtlAboveMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "256", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

TEST_F(ToolParseArgsTest, TosAboveMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-Q", "256", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

TEST_F(ToolParseArgsTest, IdentAboveMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-e", "65536", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

TEST_F(ToolParseArgsTest, PreloadAboveMax) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-l", "65537", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

TEST_F(ToolParseArgsTest, NonNumericTtl) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-t", "abc", "host"}, &argc, &argv);
  test_err_jmp_buf_set = 1;
  if (setjmp(test_err_jmp_buf) == 0) {
    tool_parse_args(&argc, &argv, &config);
    FAIL() << "Expected error() to be called";
  }
  EXPECT_EQ(last_error_status, 1);
}

/* ─── E. 不明フラグ ───────────────────────────────────────── */

TEST_F(ToolParseArgsTest, UnknownFlagReturns2) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-x", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 2);
}

TEST_F(ToolParseArgsTest, HelpFlagReturns2) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-h", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 2);
}

/* ─── F. argc/argv の調整 ─────────────────────────────────── */

TEST_F(ToolParseArgsTest, ArgvAdjustedToRemainingArgs) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-v", "-c", "3", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "host");
}

TEST_F(ToolParseArgsTest, NoOptionsLeavesHostname) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "host");
}

TEST_F(ToolParseArgsTest, MultipleOptionsAndHostname) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-v", "-A", "-D", "-t", "128", "-c", "10", "8.8.8.8"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  EXPECT_EQ(config.opt_verbose, 1u);
  EXPECT_EQ(config.opt_adaptive, 1u);
  EXPECT_EQ(config.opt_ptimeofday, 1u);
  EXPECT_EQ(config.ttl, 128);
  EXPECT_EQ(config.count, 10);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "8.8.8.8");
}

/* ─── G. デフォルト値が変化しないことの確認 ─────────────── */

TEST_F(ToolParseArgsTest, UnrelatedFieldsUnchanged) {
  int argc;
  char **argv;
  make_argv({"ft_ping", "-v", "host"}, &argc, &argv);
  EXPECT_EQ(tool_parse_args(&argc, &argv, &config), 0);
  /* -v 以外はデフォルト値のまま */
  EXPECT_EQ(config.datalen, 56);
  EXPECT_EQ(config.ttl, 64);
  EXPECT_EQ(config.count, 0);
  EXPECT_EQ(config.preload, 0);
}
