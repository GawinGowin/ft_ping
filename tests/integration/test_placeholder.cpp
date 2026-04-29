#include <gtest/gtest.h>

/* 統合テスト本体（test_ping_localhost.cpp / test_ping_options.cpp）は
 * 別 Issue で実装する。CAP_NET_RAW が必要な実ネットワーク試験のため、
 * 今回はディレクトリと CMake 構成のみを用意する。 */
TEST(IntegrationPlaceholder, Stub) { SUCCEED(); }
