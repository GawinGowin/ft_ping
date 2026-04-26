#include "ft_ping.h"
#include "tool_getparam.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static t_ping_session *g_session = NULL;

static void handle_sigint(int sig) {
  (void)sig;
  if (g_session) {
    ftping_stop(g_session);
  }
  // 2回目以降のSIGINTで強制終了できるようにデフォルトに戻す
  signal(SIGINT, SIG_DFL);
}

static void handle_sigalrm(int sig) {
  (void)sig;
  if (g_session) {
    ftping_stop(g_session);
  }
}

int main(int argc, char **argv) {
  t_ping_config config;
  ftping_config_init(&config);

  int parse_err = tool_parse_args(&argc, &argv, &config);
  if (parse_err != 0) {
    if (parse_err == 2) {
      fprintf(stderr, "Usage error\n");
    }
    return parse_err;
  }

  if (argc < 1) {
    fprintf(stderr, "ft_ping: missing host operand\n");
    return 1;
  }

  config.hostname = argv[0];
  t_ping_session *session = ftping_init(&config, config.hostname);
  if (!session) {
    fprintf(stderr, "ft_ping: failed to initialize session\n");
    return 1;
  }
  g_session = session;

  // sigaction を使用してシグナルハンドラーを設定
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; // SA_RESTART を含めない
  sigaction(SIGINT, &sa, NULL);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigalrm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

  fprintf(stderr, "[DEBUG] Starting ping with count=%ld\n", config.count);
  fflush(stderr);
  ftping_run(session);
  fprintf(stderr, "[DEBUG] ftping_run completed\n");
  fflush(stderr);

  t_ftping_stats stats = ftping_get_stats(session);

  // 統計情報の表示
  printf("\n--- %s ping statistics ---\n", config.hostname);
  printf("%d packets transmitted, %d received, ", stats.ntransmitted, stats.nreceived);
  if (stats.ntransmitted > 0) {
    int loss = ((stats.ntransmitted - stats.nreceived) * 100) / stats.ntransmitted;
    printf("%d%% packet loss, time %.0fms\n", loss, stats.tsum);
  } else {
    printf("\n");
  }

  if (stats.nreceived > 0) {
    printf(
        "rtt min/avg/max = %.3f/%.3f/%.3f ms\n", (double)stats.tmin / 1000.0,
        (stats.tsum / stats.nreceived) / 1000.0, (double)stats.tmax / 1000.0);
  }

  ftping_cleanup(session);
  return 0;
}
