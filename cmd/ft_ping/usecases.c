#include "ft_ping.h"

int parse_arg_usecase(int *argc, char ***argv, t_ping_state *state) {
  int ch;
  while ((ch = getopt(*argc, *argv, "hvc:s:")) != EOF) {
    switch (ch) {
    case 'v':
      state->opt_verbose = 1;
      break;
    case 'c':
      state->npackets = parse_long(optarg, "invalid argument", 0, LONG_MAX, error);
      break;
    case 's':
      state->datalen = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    default:
      return (2);
    }
  }
  *argc -= optind;
  *argv += optind;
  return (0);
}

void show_usage_usecase(void) {
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