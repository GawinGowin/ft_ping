#include "ft_ping.h"

static void usage();
static void signal_handler(int signo);
static void configure_state(t_ping_state *state);

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
      usage();
    }
    return (err);
  }

  if (argc != 1) {
    error(2, "usage error: only one destination address required\n");
  }
  return (0);
}

static void usage() {
  char usage_msg[] = {"\nUsage: %s [-v] destination\n\n"
                      "Options:\n"
                      "<destination>      dns name or ip address\n"
                      "-v                 verbose output\n"
                      "-c <count>         stop after <count> replies\n"
                      "-s <size>          use <size> as number of data bytes to be sent\n"
                      "-h                 display this help and exit\n"
                      "\n"
                      "For more details see https://github.com/GawinGowin/ft_ping.git\n"};
  fprintf(stderr, usage_msg, program_invocation_short_name);
}

static void signal_handler(int signo) {
  if (signo == SIGINT) {
    printf("SIGINT\n");
  }
}

static void configure_state(t_ping_state *state) {
  state->sockfd = -1;
  state->datalen = 56;
  state->npackets = 4;
  state->opt_verbose = 0;
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif