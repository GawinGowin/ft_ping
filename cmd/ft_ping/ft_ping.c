#include "ft_ping.h"
#include "icmp.h"

static void main_loop(t_ping_master *master, void *packet_ptr, size_t packet_size);
static int pinger(t_ping_master *master, void *packet, size_t packet_size);

volatile int g_is_exiting = 0;

t_ping_state *global_state = NULL;

int entrypoint(int argc, char **argv) {
  t_ping_master master;
  configure_state_usecase(&master);
  t_ping_state state = {.in_pr_addr = 0, .pr_addr_jmp = {}};
  global_state = &state;
  if (on_exit(cleanup_usecase, (void *)&master) != 0) {
    error(1, "on_exit failed\n");
    return (1);
  }
  setup_signal_handlers_usecase();
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
  size_t packet_size = sizeof(t_icmp) - sizeof(uint64_t) + master.datalen;
  printf(
      "PING %s (%s) %d(%zu) bytes of data.\n", master.hostname, inet_ntoa(master.whereto.sin_addr),
      master.datalen, packet_size + IPV4_HEADER_SIZE);
  void *packet = malloc(packet_size);
  if (!packet) {
    error(1, "malloc failed\n");
  }
  bzero(packet, packet_size);
  main_loop(&master, packet, packet_size);
  free(packet);
  return (0);
}

static void main_loop(t_ping_master *master, void *packet_ptr, size_t packet_size) {
  int next;
  int polling;
  int recv_error;
  void *recved_packet = malloc(packet_size);
  if (!recved_packet) {
    error(1, "malloc failed failed\n");
  }
  bzero(recved_packet, packet_size);
  while (!g_is_exiting) {
    do {
      next = pinger(master, packet_ptr, packet_size);
      next = schedule_exit(master, next);
    } while (next <= 0 && !g_is_exiting);
    polling = 0;
    recv_error = 0;
    if (master->opt_adaptive || master->opt_flood_poll || next < SCHINT(master->interval)) {
      int recv_expected = master->ntransmitted - master->nreceived;

      if (1000 % HZ == 0 ? next <= 1000 / HZ : (next < INT_MAX / HZ && next * HZ <= 1000)) {
        if (recv_expected) {
          next = MIN_INTERVAL_MS;
        } else {
          next = 0;
          polling = MSG_DONTWAIT;
          sched_yield();
        }
      }
      if (!polling && (master->opt_adaptive || master->opt_flood_poll || master->interval)) {
        struct pollfd pset;
        pset.fd = master->sockfd;
        pset.events = POLLIN;
        pset.revents = 0;
        if (poll(&pset, 1, next) < 1 || !(pset.revents & (POLLIN | POLLERR)))
          continue;
        polling = MSG_DONTWAIT;
        recv_error = pset.revents & POLLERR;
      }
    }
    receive_replies_usecase(master, recved_packet, packet_size, &polling, &recv_error);
  }
  free(recved_packet);
  g_is_exiting = 0;
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

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
