#include "ft_ping.h"

int parse_arg_usecase(int *argc, char ***argv, t_ping_state *state) {
  int ch;
  while ((ch = getopt(*argc, *argv, "hvc:s:")) != EOF) {
    switch (ch) {
    case 'v':
      state->opt_verbose = 1;
      break;
    case 'c':
      state->npackets = parse_long(optarg, "invalid argument", 0, LONG_MAX);
      break;
    case 's':
      state->datalen = parse_long(optarg, "invalid argument", 0, INT_MAX);
      break;
    default:
      return (2);
    }
  }
  *argc -= optind;
  *argv += optind;
  return (0);
}
