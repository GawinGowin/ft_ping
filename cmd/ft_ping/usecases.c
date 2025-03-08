#include "ft_ping.h"

int initialize_usecase(t_ping_state *state, char **argv) {
  char *target = *argv;
  state->hostname = target;

  if (is_ipv6_address(target)) {
    error(2, "IPv6 is not supported\n");
  }

  // ソケット作成
  state->sockfd = create_socket_with_fallback();
  if (state->sockfd < 0) {
    error(1, "Failed to create socket: %s\n", strerror(errno));
  }
  // ソケットオプション
  if (setsockopt(state->sockfd, SOL_SOCKET, SO_SNDBUF, &state->sndbuf, sizeof(state->sndbuf)) < 0) {
    error(1, "setsockopt SO_SNDBUF failed: %s\n", strerror(errno));
  }
  if (setsockopt(state->sockfd, SOL_SOCKET, SO_RCVBUF, &state->rcvbuf, sizeof(state->rcvbuf)) < 0) {
    error(1, "setsockopt SO_RCVBUF failed: %s\n", strerror(errno));
  }
  if (setsockopt(state->sockfd, IPPROTO_IP, IP_TTL, &state->ttl, sizeof(state->ttl)) < 0) {
    error(1, "setsockopt IP_TTL failed: %s\n", strerror(errno));
  }
  if (setsockopt(state->sockfd, IPPROTO_IP, IP_TOS, &state->tos, sizeof(state->tos)) < 0) {
    error(1, "setsockopt IP_TOS failed: %s\n", strerror(errno));
  }
  int on = 1;
  if (setsockopt(state->sockfd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) < 0) {
    error(1, "setsockopt IP_RECVERR failed: %s\n", strerror(errno));
  }
  dns_lookup(target, &state->whereto);
  return (0);
}

int parse_arg_usecase(int *argc, char ***argv, t_ping_state *state) {
  int ch;
  while ((ch = getopt(*argc, *argv, "hvt:Q:c:s:")) != EOF) {
    switch (ch) {
    case 'v':
      state->opt_verbose = 1;
      break;
    case 't':
      state->ttl = parse_long(optarg, "invalid argument", 1, 255, error);
      break;
    case 'Q':
      state->tos = parse_long(optarg, "invalid argument", 0, 255, error);
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

int send_ping_usecase(void *packet, int datalen, int sockfd, struct sockaddr_in *whereto) {
  int cc = sendto(
      sockfd, packet, sizeof(t_icmp) + datalen, 0, (struct sockaddr *)whereto, sizeof(*whereto));
  if (cc < 0) {
    if (errno == ENOBUFS || errno == ENOMEM) {
      return -1;
    }
    error(0, "sendto: %s\n", strerror(errno));
    return -1;
  }
  return cc;
}

void show_usage_usecase(void) {
  char usage_msg[] = {"\nUsage:\n  %s [options] <destination>\n\n"
                      "Options:\n"
                      "  <destination>      dns name or ip address\n"
                      "  -c <count>         stop after <count> replies\n"
                      "  -h                 display this help and exit\n"
                      "  -Q <tclass>        use quality of service <tclass> bits\n"
                      "  -s <size>          use <size> as number of data bytes to be sent\n"
                      "  -t <ttl>           define time to live\n"
                      "  -v                 verbose output\n"
                      "\n"
                      "For more details see https://github.com/GawinGowin/ft_ping.git\n"};
  fprintf(stderr, usage_msg, program_invocation_short_name);
}

void cleanup_usecase(int status, void *state) {
  (void)status;
  t_ping_state *st = (t_ping_state *)state;
  close(st->sockfd);
}
