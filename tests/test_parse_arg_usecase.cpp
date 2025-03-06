#include <gtest/gtest.h>
#include <limits.h>

extern "C" {
#include "ft_ping.h"
}

class ParseArgUsecaseTest : public ::testing::Test {
protected:
  t_ping_state state;
  char **test_argv;
  int test_argc;

  void SetUp() override {
    memset(&state, 0, sizeof(t_ping_state));
    optind = 1;
    opterr = 1;
    optopt = 0;
  }

  void TearDown() override {
    for (int i = 0; i < test_argc; i++) {
      free(test_argv[i]);
    }
    free(test_argv);
  }

  void CreateArgv(const std::vector<std::string> &args) {
    test_argc = args.size();
    test_argv = (char **)malloc(sizeof(char *) * test_argc);

    for (int i = 0; i < test_argc; i++) {
      test_argv[i] = strdup(args[i].c_str());
    }
  }
};

// -v フラグのテスト
TEST_F(ParseArgUsecaseTest, VerboseFlag) {
  CreateArgv({"ft_ping", "-v", "example.com"});

  int argc = test_argc;
  char **argv = test_argv;

  int result = parse_arg_usecase(&argc, &argv, &state);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(state.opt_verbose, 1);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "example.com");
}

// -c フラグのテスト
TEST_F(ParseArgUsecaseTest, CountFlag) {
  CreateArgv({"ft_ping", "-c", "5", "example.com"});

  int argc = test_argc;
  char **argv = test_argv;

  int result = parse_arg_usecase(&argc, &argv, &state);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(state.npackets, 5);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "example.com");
}

// -s フラグのテスト
TEST_F(ParseArgUsecaseTest, DataSizeFlag) {
  CreateArgv({"ft_ping", "-s", "64", "example.com"});

  int argc = test_argc;
  char **argv = test_argv;

  int result = parse_arg_usecase(&argc, &argv, &state);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(state.datalen, 64);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "example.com");
}

// 複数フラグのテスト
TEST_F(ParseArgUsecaseTest, MultipleFlags) {
  CreateArgv({"ft_ping", "-v", "-c", "3", "-s", "32", "example.com"});

  int argc = test_argc;
  char **argv = test_argv;

  int result = parse_arg_usecase(&argc, &argv, &state);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(state.opt_verbose, 1);
  EXPECT_EQ(state.npackets, 3);
  EXPECT_EQ(state.datalen, 32);
  EXPECT_EQ(argc, 1);
  EXPECT_STREQ(argv[0], "example.com");
}

// 不正なフラグのテスト
TEST_F(ParseArgUsecaseTest, InvalidFlag) {
  CreateArgv({"ft_ping", "-x", "example.com"});

  int argc = test_argc;
  char **argv = test_argv;

  int result = parse_arg_usecase(&argc, &argv, &state);

  EXPECT_EQ(result, 2); // デフォルトケースで2が返される
}
