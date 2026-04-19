
#include "tool_getparam.h"

// tool_show_usage()

int tool_parse_args(int *argc, char ***argv, t_ping_config *config) {
  int ch;
  while ((ch = getopt(*argc, *argv, "AhvDe:t:Q:c:S:s:l:w:")) != EOF) {
    switch (ch) {
    case 'A':
      config->opt_adaptive = 1;
      break;
    case 'v':
      config->opt_verbose = 1;
      break;
    case 'D':
      config->opt_ptimeofday = 1;
      break;
    case 't':
      config->ttl = parse_long(optarg, "invalid argument", 1, 255, error);
      break;
    case 'Q':
      config->tos = parse_long(optarg, "invalid argument", 0, 255, error);
      break;
    case 'c':
      config->count = parse_long(optarg, "invalid argument", 0, LONG_MAX, error);
      break;
    case 'e':
      config->ident = htons((uint16_t)parse_long(optarg, "invalid argument", 0, 0xFFFF, error));
      break;
    case 'S':
      config->sndbuf = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 's':
      config->datalen = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 'l':
      config->preload = parse_long(optarg, "invalid argument", 0, 65536, error);
      break;
    case 'w':
      config->deadline_sec = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    default:
      return (2);
    }
  }
  *argc -= optind;
  *argv += optind;
  return (0);
}
