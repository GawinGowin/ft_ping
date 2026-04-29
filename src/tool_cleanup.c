#include "tool_cleanup.h"
#include "tool_signal.h"

#include <stdlib.h>

static t_ping_session *s_session = NULL;
static int s_done = 0;

static void tool_cleanup_atexit(void) {
  if (s_session && !s_done) {
    tool_cleanup(s_session);
  }
}

void tool_cleanup_register(t_ping_session *session) {
  if (s_session)
    return;
  s_session = session;
  atexit(tool_cleanup_atexit);
}

void tool_cleanup(t_ping_session *session) {
  if (s_done)
    return;
  s_done = 1;
  tool_signal_teardown();
  if (session)
    ftping_cleanup(session);
  s_session = NULL;
}
