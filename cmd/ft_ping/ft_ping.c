#include "ft_ping.h"
#include "icmp.h"

static void configure_state(t_ping_state *state);

static void signal_handler(int signo) {
  if (signo == SIGINT) {
    printf("SIGINT\n");
  }
}

static void main_loop(t_ping_state *state);

int entrypoint(int argc, char **argv) {
  t_ping_state state;
  configure_state(&state);

  if (on_exit(cleanup_usecase, (void *)&state) != 0) {
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
    error(2, "usage error: Destination address required\n");
  }
  initialize_usecase(&state, argv);
  main_loop(&state);
  return (0);
}

static void main_loop(t_ping_state *state) {
  size_t packet_size = sizeof(t_icmp) - sizeof(uint64_t) + state->datalen;
  printf(
      "PING %s (%s):  %d(%zu) bytes of data.\n", state->hostname,
      inet_ntoa(state->whereto.sin_addr), state->datalen, packet_size);
  void *packet = malloc(packet_size);
  // pingループ (このループ内でexitをしてはならない)
  // TODO: SIGINTで終了できるようにする
  for (int i = 0; i < state->npackets || state->npackets <= 0; i++) {
    create_echo_request_packet(packet, 0, i);
    generate_packet_data(packet, state->datalen);
    set_timestamp(packet);
    t_icmp *icmp = (t_icmp *)packet;
    icmp->checksum = 0;
    icmp->checksum = calculate_checksum(packet, packet_size);
    int cc = send_ping_usecase(packet, packet_size, state->sockfd, &state->whereto);
    if (cc < 0) {
      if (errno == ENOBUFS) {
        printf("Device busy\n");
        usleep(10000);
        continue;
      }
    }
    // TODO: レスポンス待機
    usleep(1000000);
  }
  free(packet);
}

static void configure_state(t_ping_state *state) {
  // temp values
  state->sockfd = -1;
  state->datalen = 56;
  state->sndbuf = 64 * 1024;
  state->rcvbuf = 64 * 1024;
  state->ttl = 64;
  state->tos = 0;

  state->npackets = -1;
  state->opt_verbose = 0;
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
