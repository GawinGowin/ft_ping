#include "ft_ping.h"

static void configure_state(t_ping_state *state);

static void signal_handler(int signo) {
  if (signo == SIGINT) {
    printf("SIGINT\n");
  }
}

int entrypoint(int argc, char **argv) {
  t_ping_state state;
  configure_state(&state);

  if (on_exit(cleanup, (void *)&state) != 0) {
    error(1, "on_exit failed\n");
    return (1);
  }
  if (signal(SIGINT, signal_handler) == SIG_ERR || signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
    error(1, "signal failed\n");
  }
  int err;
  if ((err = parse_arg_usecase(&argc, &argv, &state)) != 0) {
    if (err == 2) {
      show_usage_usecase();
    }
    return (err);
  }

  if (argc != 1) {
    error(2, "usage error: only one destination address required\n");
  }
  init(&state, argv);
  // main_loop(&state, argv[0]);
  return (0);
}

// void main_loop(t_ping_state *state, const char *dest) {
//   long count = state->npackets;
//   for (long i = 0; i < count; i++) {
//     send_ping(state, dest);
//     recv_ping(state);
//   }
//   if (state->opt_verbose) {
//     printf("PING %s (%s) %d(%d) bytes of data.\n", dest, dest, state->datalen, state->datalen + 8);
//   }
// }

static void configure_state(t_ping_state *state) {
  state->sockfd = -1;
  state->datalen = 56;
  state->npackets = 4;
  state->opt_verbose = 0;
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
