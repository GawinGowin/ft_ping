#include "ping_loop.h"

#define MIN_INTERVAL_MS 10
#define SCHINT(a) (((a) <= MIN_INTERVAL_MS) ? MIN_INTERVAL_MS : (a))

/* 適応的ping/floodモードでfastパスを使うか判定 */
static int should_use_fast_path(const t_ping_config *config, int next) {
  return config->opt_adaptive || config->opt_flood_poll || next < SCHINT(config->interval_ms);
}

static int wait_for_reply(int fd, int next, int *polling, int *recv_error) {
  struct pollfd pset;
  pset.fd = fd;
  pset.events = POLLIN;
  pset.revents = 0;
  if (poll(&pset, 1, next) < 1 || !(pset.revents & (POLLIN | POLLERR)))
    return 0;
  *polling = MSG_DONTWAIT;
  *recv_error = pset.revents & POLLERR;
  return 1;
}

void ping_run(t_ping_session *session) {
  int next;
  int polling;
  int recv_error;

  t_ping_config *config = &session->config;
  t_socket_st *sock_st = &session->net.socket_state;

  size_t packet_size = sock_st->ops->packet_size(config->datalen);
  void *send_packet = malloc(packet_size);
  if (!send_packet)
    error(1, "malloc failed\n");
  memset(send_packet, 0, packet_size);

  void *recv_buf = malloc(packet_size);
  if (!recv_buf) {
    free(send_packet);
    error(1, "malloc failed\n");
  }
  memset(recv_buf, 0, packet_size);

  struct iovec iov;
  struct msghdr msg;
  t_ping_receive received = {
      .socket_fd = &sock_st->fd,
      .packlen = packet_size,
      .polling = &polling,
      .iov = &iov,
      .msg = &msg,
  };
  received.iov->iov_base = recv_buf;

  while (1) {
    if (session->is_exiting)
      break;
    if (config->count && session->stats.nreceived >= config->count)
      break;

    do {
      next = ping_send_one(session, send_packet, packet_size);
      /* TODO: schedule_exit を ping_schedule.c から呼ぶ */
    } while (next <= 0);

    polling = 0;
    recv_error = 0;

    if (should_use_fast_path(config, next)) {
      int recv_expected = session->stats.ntransmitted - session->stats.nreceived;
      long hz = sysconf(_SC_CLK_TCK);

      if (1000 % hz == 0 ? next <= 1000 / hz : (next < __INT_MAX__ / hz && next * hz <= 1000)) {
        if (recv_expected) {
          next = MIN_INTERVAL_MS;
        } else {
          next = 0;
          polling = MSG_DONTWAIT;
          sched_yield();
        }
      }
      if (!polling && (config->opt_adaptive || config->opt_flood_poll || config->interval_ms)) {
        if (!wait_for_reply(sock_st->fd, next, &polling, &recv_error))
          continue;
      }
    }
    ping_receive_replies(session, &received);
    (void)recv_error;
  }

  free(send_packet);
  free(recv_buf);
}

#ifdef TESTING
int should_use_fast_path_test(const t_ping_config *config, int next) {
  return should_use_fast_path(config, next);
}
#endif

int ping_send_one(t_ping_session *session, void *packet, size_t packet_size) {
  t_ping_config *config = &session->config;
  t_ping_net_state *net = &session->net;
  t_ping_timer *timer = &session->timer;

  if (config->count > 0 && session->stats.ntransmitted >= config->count)
    return 0;

  uint16_t seq = (uint16_t)(session->stats.ntransmitted % UINT16_MAX);

  if (timer->prev_send_time.tv_sec == 0 && timer->prev_send_time.tv_usec == 0) {
    gettimeofday(&timer->prev_send_time, NULL);
    t_ipheader_ctx ctx = {
        .seq = seq,
        .datalen = config->datalen,
        .ts = timer->prev_send_time,
        .src = net->from.sin_addr,
        .dst = net->whereto.sin_addr,
    };
    net->socket_state.ops->build_ipheader(packet, &ctx);
    if (send_packet(packet, packet_size, net->socket_state.fd, &net->whereto) < 0)
      return -1;
    session->stats.ntransmitted++;
    return config->interval_ms;
  }

  struct timeval now;
  gettimeofday(&now, NULL);
  long delta_ms = (now.tv_sec - timer->prev_send_time.tv_sec) * 1000 +
                  (now.tv_usec - timer->prev_send_time.tv_usec) / 1000;
  if (delta_ms < config->interval_ms)
    return config->interval_ms - (int)delta_ms;

  timer->prev_send_time = now;
  t_ipheader_ctx ctx = {
      .seq = seq,
      .datalen = config->datalen,
      .ts = timer->prev_send_time,
      .src = net->from.sin_addr,
      .dst = net->whereto.sin_addr,
  };
  net->socket_state.ops->build_ipheader(packet, &ctx);
  if (send_packet(packet, packet_size, net->socket_state.fd, &net->whereto) < 0)
    return -1;
  session->stats.ntransmitted++;
  return config->interval_ms;
}

int ping_receive_replies(t_ping_session *session, t_ping_receive *received) {
  if (!received)
    return 0;

  struct msghdr *msg = received->msg;
  ssize_t ret = 0;

  while (1) {
    struct timeval recv_time;

    received->iov->iov_len = received->packlen;
    memset(msg, 0, sizeof(*msg));
    msg->msg_name = &received->addrbuf;
    msg->msg_namelen = sizeof(received->addrbuf);
    msg->msg_iov = received->iov;
    msg->msg_iovlen = 1;
    msg->msg_control = &received->ans_data;
    msg->msg_controllen = sizeof(received->ans_data);

    ret = recvmsg(*received->socket_fd, msg, *received->polling);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      return (int)ret;
    }

    gettimeofday(&recv_time, NULL);

    struct sockaddr_in *from = (struct sockaddr_in *)msg->msg_name;
    int icmp_len = 0;
    struct icmphdr *icmp =
        session->net.socket_state.ops->extract_icmp(received->iov->iov_base, ret, &icmp_len);

    if (icmp)
      ping_stats_gather(
          &session->stats, icmp->un.echo.sequence,
          /* triptime */ 0, 0);

    (void)from;
    *received->polling = MSG_DONTWAIT;
    break;
  }
  return (int)ret;
}

int ping_init(t_ping_session *session, char *target) {
  t_ping_net_state *net = &session->net;
  t_ping_config *config = &session->config;

  net->ident = (uint16_t)(getpid() & 0xFFFF);

  if (ping_socket_select(&net->socket_state) < 0)
    error(1, "Failed to create socket: %s\n", strerror(errno));

  int fd = net->socket_state.fd;

  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &config->ttl, sizeof(config->ttl)) < 0)
    error(1, "setsockopt IP_TTL failed: %s\n", strerror(errno));
  if (setsockopt(fd, IPPROTO_IP, IP_TOS, &config->tos, sizeof(config->tos)) < 0)
    error(1, "setsockopt IP_TOS failed: %s\n", strerror(errno));
  int on = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) < 0)
    error(1, "setsockopt IP_RECVERR failed: %s\n", strerror(errno));

  net->socket_state.ops->extra_configure(fd);

  dns_lookup(target, &net->whereto);
  get_source_address(&net->from, &net->whereto, NULL);
  return 0;
}
