#include "ft_ping.h"

t_ping_state *global_state = NULL;

static void main_loop(t_ping_master *master, void *packet_ptr, size_t packet_size);
static int pinger(t_ping_master *master, void *packet, size_t packet_size);

int entrypoint(int argc, char **argv) {
  t_ping_master master;
  configure_state_usecase(&master);

  // on_exitでスコープ外部からメモリをfreeできるようにするため
  static t_ping_state state = {
      .is_in_printing_addr = 0,
      .is_exiting = 0,
      .pr_addr_jmp = {},
      .allocated_packet_addr = NULL,
      .socket_state = NULL};
  global_state = &state;

  if (on_exit(cleanup_usecase, (void *)global_state) != 0) {
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
  global_state->socket_state = &master.socket_state;

  size_t packet_size = sizeof(struct icmphdr) + master.datalen;
  int is_raw_socket = (master.socket_state.socktype == SOCK_RAW);
  if (is_raw_socket) {
    packet_size += sizeof(struct iphdr);
  }
  printf(
      "PING %s (%s) %d(%zu) bytes of data.\n", master.hostname, inet_ntoa(master.whereto.sin_addr),
      master.datalen, is_raw_socket ? packet_size : packet_size + sizeof(struct iphdr));
  global_state->allocated_packet_addr = malloc(packet_size);
  void *packet = global_state->allocated_packet_addr;
  if (!packet) {
    error(1, "malloc failed\n");
  }
  bzero(packet, packet_size);
  main_loop(&master, packet, packet_size);
  free(global_state->allocated_packet_addr);
  global_state->allocated_packet_addr = NULL;
  return (0);
}

static void main_loop(t_ping_master *master, void *packet_ptr, size_t packet_size) {
  int next;
  int polling;
  int recv_error;

  // #TODO: ここのアドレスをon_exitで解放できるようにglobal_stateで管理したい
  void *recved_packet = malloc(packet_size);
  if (!recved_packet) {
    error(1, "malloc failed\n");
  }
  struct iovec iov;
  struct msghdr msg;
  t_receive_replies_dto receive_replies_dto = {
      .socket_fd = &master->socket_state.fd,
      .packlen = packet_size,
      .polling = &polling,
      .iov = &iov,
      .msg = &msg,
      .master = master};
  receive_replies_dto.iov->iov_base = (char *)recved_packet;

  bzero(recved_packet, packet_size);
  while (1) {
    if (global_state->is_exiting) {
      break;
    }
    if (master->npackets && master->nreceived >= master->npackets)
      break;
    /* status_snapshot を実装する */
    // if (rts->status_snapshot)
    // 	status(rts);
    do {
      next = pinger(master, packet_ptr, packet_size);
      next = schedule_exit(master, next);
    } while (next <= 0);
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
        pset.fd = master->socket_state.fd;
        pset.events = POLLIN;
        pset.revents = 0;
        if (poll(&pset, 1, next) < 1 || !(pset.revents & (POLLIN | POLLERR)))
          continue;
        polling = MSG_DONTWAIT;
        recv_error = pset.revents & POLLERR;
      }
    }
    receive_replies_usecase(&receive_replies_dto);
    (void) recv_error;
  }
  free(recved_packet);
  recved_packet = NULL;
  // 統計情報を表示
  finish_statistics_usecase(master);
  return;
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
        &(master->socket_state), &master->from, &master->whereto, packet, packet_size,
        master->datalen, seq, &prev);
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
      &(master->socket_state), &master->from, &master->whereto, packet, packet_size,
      master->datalen, seq, &prev);
  if (cc < 0) {
    return -1;
  }
  master->ntransmitted++;
  return master->interval;
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
