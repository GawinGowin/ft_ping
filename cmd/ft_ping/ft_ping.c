#include "ft_ping.h"
#include "icmp.h"

static void main_loop(t_ping_master *master);
static int pinger(t_ping_master *master, void *packet, size_t packet_size);
static void configure_state(t_ping_master *master);
static void alarm_handler(int signo);

volatile int g_is_exiting = 0;

static void signal_handler(int signo) {
  if (signo == SIGINT) {
    g_is_exiting = 1;
  }
}

static void alarm_handler(int signo) {
  if (signo == SIGALRM) {
    g_is_exiting = 1;
  }
}

int entrypoint(int argc, char **argv) {
  t_ping_master master;

  configure_state(&master);
  if (on_exit(cleanup_usecase, (void *)&master) != 0) {
    error(1, "on_exit failed\n");
    return (1);
  }
  if (signal(SIGINT, signal_handler) == SIG_ERR || signal(SIGQUIT, SIG_IGN) == SIG_ERR ||
      signal(SIGALRM, alarm_handler) == SIG_ERR) {
    error(1, "signal failed\n");
  }
  int err;
  if ((err = parse_arg_usecase(&argc, &argv, &master)) != 0) {
    if (err == 2) {
      show_usage_usecase();
    }
    return (err);
  }
  if (argc != 1) {
    error(2, "usage error: Destination address required\n");
  }
  initialize_usecase(&master, argv);
  main_loop(&master);
  return (0);
}

static void main_loop(t_ping_master *master) {
  size_t packet_size = sizeof(t_icmp) - sizeof(uint64_t) + master->datalen;
  printf(
      "PING %s (%s) %d(%zu) bytes of data.\n", master->hostname,
      inet_ntoa(master->whereto.sin_addr), master->datalen, packet_size + IPV4_HEADER_SIZE);
  void *packet = malloc(packet_size);
  if (!packet) {
    error(1, "malloc failed\n");
  }

  int next;
  do {
    next = pinger(master, packet, packet_size);
    next = schedule_exit(master, next);
  } while (next <= 0 && !g_is_exiting);

  if (!g_is_exiting && next > 0) {
    struct pollfd pfd;
    pfd.fd = master->sockfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, next);
    if (ret > 0 && (pfd.revents & POLLIN)) {

    }
  }

  free(packet);
}

static int pinger(t_ping_master *master, void *packet, size_t packet_size) {
  static struct timeval prev = {0, 0};
  static struct timeval now;
  long time_delta;

  // 送信完了チェック（-cオプション指定時）
  if (master->npackets > 0 && master->ntransmitted >= master->npackets) {
    return 0; // 送信完了
  }

  uint16_t seq = (uint16_t)(master->ntransmitted % UINT16_MAX);
  if (prev.tv_sec == 0 && prev.tv_usec == 0) {
    gettimeofday(&prev, NULL);
    int cc = send_ping_usecase(
        master->sockfd, &master->whereto, packet, packet_size, master->datalen, seq, &prev);
    if (cc < 0) {
      // Note: 本物のpingでは送信間隔の調整など複雑なエラーハンドリングが行われていた
      return -1;
    }
    master->ntransmitted++;
    return master->interval;
  }
  gettimeofday(&now, NULL);
  time_delta = (now.tv_sec - prev.tv_sec) * 1000 + (now.tv_usec - prev.tv_usec) / 1000;
  if (time_delta < master->interval) {
    return master->interval - time_delta;
  }
  memcpy(&prev, &now, sizeof(struct timeval));
  int cc = send_ping_usecase(
      master->sockfd, &master->whereto, packet, packet_size, master->datalen, seq, &prev);
  if (cc < 0) {
    return -1;
  }
  master->ntransmitted++;
  return master->interval;
}

static void configure_state(t_ping_master *master) {
  // temp values
  master->sockfd = -1;
  master->datalen = 56;
  master->ttl = 64;
  master->tos = 0;
  master->preload = 1;
  master->sndbuf = 0;
  master->interval = 1000;
  master->npackets = -1;
  master->opt_adaptive = 0;
  master->opt_verbose = 0;
  master->deadline = 0;
  master->ntransmitted = 0;
  master->nreceived = 0;
  master->tmax = 0;
  master->lingertime = 10;
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
