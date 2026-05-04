
#include "tool_getparam.h"

#include <stdlib.h>

static void show_usage(void);

int tool_parse_args(int *argc, char ***argv, t_ping_config *config) {
  int ch;
  while ((ch = getopt(*argc, *argv, "h?AhvDe:t:Q:c:S:s:l:w:")) != EOF) {
    switch (ch) {
    case 'A': // TODO: -A                 use adaptive ping
      config->opt_adaptive = 1;
      break;
    case 'v': // TODO: -v                 verbose output
      config->opt_verbose = 1;
      break;
    case 'D': // -D                 print timestamps
      config->opt_ptimeofday = 1;
      break;
    case 't': // -t <ttl>           define time to live
      config->ttl = parse_long(optarg, "invalid argument", 1, 255, error);
      break;
    case 'Q': // -Q <tclass>        use quality of service <tclass> bits
      config->tos = parse_long(optarg, "invalid argument", 0, 255, error);
      break;
    case 'c': // -c <count>         stop after <count> replies
      config->count = parse_long(optarg, "invalid argument", 0, LONG_MAX, error);
      break;
    case 'e': // -e <identifier>    define identifier for ping session,
      config->ident = (uint16_t)parse_long(optarg, "invalid argument", 0, 0xFFFF, error);
      config->opt_useident = 1;
      break;
    case 'S': // -S <size>          use <size> as SO_SNDBUF socket option value
      config->sndbuf = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 's': // -s <size>          use <size> as number of data bytes to be sent
      config->datalen = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 'l': // TODO: -l <preload>       send <preload> number of packages while waiting replies
      config->preload = parse_long(optarg, "invalid argument", 0, 65536, error);
      if (getuid() == 0 && config->preload > 3) {
        error(2, "cannot set preload to value greater than 3: %ld", config->preload);
      }
      break;
    case 'w': // -w <deadline>      reply wait <deadline> in seconds
      config->deadline_sec = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    default:
      show_usage();
      return 2;
    }
  }
  *argc -= optind;
  *argv += optind;
  return 0;
}

static void show_usage(void) {
  char usage_msg[] = {
      "\nUsage:\n  %s [options] <destination>\n\n"
      "Options:\n"
      "  <destination>      dns name or ip address\n"
      "  -A                 use adaptive ping\n"
      "  -c <count>         stop after <count> replies\n"
      "  -D                 print timestamps\n"
      "  -e <identifier>    define identifier for ping session, default is random for\n"
      "                     SOCK_RAW and kernel defined for SOCK_DGRAM\n"
      "                     Imply using SOCK_RAW (for IPv4 only for identifier 0)\n"
      "  -h                 display this help and exit\n"
      "  -l <preload>       send <preload> number of packages while waiting replies\n"
      "  -Q <tclass>        use quality of service <tclass> bits\n"
      "  -s <size>          use <size> as number of data bytes to be sent\n"
      "  -S <size>          use <size> as SO_SNDBUF socket option value\n"
      "  -t <ttl>           define time to live\n"
      "  -v                 verbose output\n"
      "  -w <deadline>      reply wait <deadline> in seconds\n"
      "\n"
      "For more details see https://github.com/GawinGowin/ft_ping.git\n"};
  fprintf(stderr, usage_msg, program_invocation_short_name);
}
