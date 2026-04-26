#include "tool_signal.h"
#include <signal.h>
#include <string.h>

static t_ping_session *s_session = NULL;

static void tool_signal_handler(int signo) {
  if (s_session) {
    ftping_stop(s_session);
  }
  if (signo == SIGINT) {
    signal(SIGINT, SIG_DFL);
  }
}

void tool_setup_signals(t_ping_session *session) {
  s_session = session;

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = tool_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGALRM, &sa, NULL);

  signal(SIGQUIT, SIG_IGN);
}
