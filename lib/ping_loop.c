#include "ping_loop.h"

#include <linux/errqueue.h>
#include <stdio.h>

#define MIN_INTERVAL_MS 10
#define SCHINT(a) (((a) <= MIN_INTERVAL_MS) ? MIN_INTERVAL_MS : (a))

/* MSG_ERRQUEUE でエラーキューから 1 件読み出す。
 * IP_RECVERR を有効にしている場合、宛先到達不能等のエラー応答や
 * ローカルエラー(EMSGSIZE 等) はメインキューではなくエラーキューに入る。
 * 読み出さないと POLLERR が立ち続けて poll が空転する原因になる。 */
static int receive_error_msg(int fd) {
  char cbuf[512];
  char buf[1024];
  struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
  struct sockaddr_in from;
  struct msghdr msg;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = &from;
  msg.msg_namelen = sizeof(from);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cbuf;
  msg.msg_controllen = sizeof(cbuf);

  ssize_t res = recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);
  if (res < 0)
    return -1;
  return (int)res;
}

/* 適応的ping/floodモードでfastパスを使うか判定 */
static int should_use_fast_path(const t_ping_config *config, int next) {
  return config->opt_adaptive || config->opt_flood_poll || next < SCHINT(config->interval_ms);
}

static int wait_for_reply(int fd, int next, int *polling, int *recv_error) {
  FILE *dbgfile = fopen("/tmp/ping_debug.log", "a");
  if (dbgfile) {
    fprintf(dbgfile, "[WAIT] poll timeout=%d\n", next);
    fflush(dbgfile);
    fclose(dbgfile);
  }
  struct pollfd pset;
  pset.fd = fd;
  pset.events = POLLIN;
  pset.revents = 0;
  int ret = poll(&pset, 1, next);
  if (ret < 0) {
    if (errno == EINTR)
      return -1;
    return 0;
  }
  if (ret == 0 || !(pset.revents & (POLLIN | POLLERR)))
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
      next = ping_schedule_exit(config, &(session->stats), &(session->timer), next);
      if (session->is_exiting)
        break;
      /* next <= 0 で再送試行: iputils main_loop と同じ条件。
       * next == -1 (送信失敗) と next == 0 を区別せず、即時に再試行する。 */
    } while (next <= 0);

    if (session->is_exiting)
      break;

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
        int ret = wait_for_reply(sock_st->fd, next, &polling, &recv_error);
        if (ret < 0) // EINTR
          continue;
        if (ret == 0)
          continue;
      }
    } else {
      // next == 0 の場合も、ブロッキング recvmsg で止まらないよう poll を挟む
      int wait_ms = (next > 0) ? next : 10;
      int ret = wait_for_reply(sock_st->fd, wait_ms, &polling, &recv_error);
      if (ret < 0) // EINTR
        continue;
      if (ret == 0) // timeout: ソケットにデータなし。次回 pinger に戻る
        continue;
    }

    if (session->is_exiting)
      break;
    if (recv_error)
      receive_error_msg(sock_st->fd);
    ping_receive_replies(session, &received);
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
      return SCHINT(config->interval_ms);
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
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ret = 0;
        break;
      }
      return (int)ret;
    }

    gettimeofday(&recv_time, NULL);

    struct sockaddr_in *from = (struct sockaddr_in *)msg->msg_name;
    int icmp_len = 0;
    struct icmphdr *icmp =
        session->net.socket_state.ops->extract_icmp(received->iov->iov_base, ret, &icmp_len);

    /* SOCK_DGRAM では kernel が echo.id を上書きするため照合不要(常に自分のもの)。
     * SOCK_RAW は他プロセスの ping 応答もカーネル経由で届くため、ident 一致を
     * 確認しないと他人の応答を自分の統計に計上してしまう。
     * iputils is_ours() (ping_common.c:1016) と同じ判定。 */
    int is_ours = session->net.socket_state.socktype == SOCK_DGRAM ||
                  ntohs(icmp ? icmp->un.echo.id : 0) == session->net.ident;

    if (icmp && is_ours)
      ping_stats_gather(
          &session->stats, icmp->un.echo.sequence,
          /* triptime */ 0, 0);

    (void)from;
    /* 2回目以降は non-blocking で連続吸い出し。
     * カーネルの受信キューに溜まっている応答を 1 ループで全て処理することで
     * in_flight を解消し、次の pinger 呼び出しを正しいタイミングに保つ。 */
    *received->polling = MSG_DONTWAIT;

    /* in_flight() == 0 なら、これ以上待ってもデータは来ないので抜ける。
     * 残っていれば EAGAIN が返るまで recvmsg を繰り返す。 */
    if (session->stats.ntransmitted - session->stats.nreceived <= 0)
      break;
  }
  return (int)ret;
}

/**
 * @brief ソケットの送受信バッファサイズを設定する。
 * @see https://github.com/GawinGowin/ft_ping/wiki/set_socket_buff
 *
 * 1パケットがカーネル内で消費するメモリを粗く見積もり、SO_SNDBUF と SO_RCVBUF を
 * 適切なサイズに設定する。iputils の sock_setbufs() (ping_common.c:443) に相当し、
 * ping.c:1034 と同じ計算式を使用する。
 *
 * ### バッファサイズの計算式
 * ```
 * send = (datalen + 8)                              // ICMPヘッダ(8B) + データ
 *      + ceil(send / 512) * (IPV4_HEADER_SIZE + 240) // sk_buff オーバーヘッド見積もり
 * ```
 * `ceil(send / 512)` はカーネルが sk_buff 単位でメモリを管理するための切り上げ。
 * `IPV4_HEADER_SIZE + 240 = 260` は iputils の `optlen + 20 + 16 + 64 + 160` と等価
 * （ft_ping は IP オプション未対応のため optlen=0 として固定）。
 *
 * ### SO_SNDBUF
 * `-S` オプション未指定時は `send`（1パケット分）を設定する。
 *
 * ### SO_RCVBUF
 * `-l preload` 個のパケットを同時に空中に飛ばせるよう `send * preload` を設定する。
 * カーネルの rmem_max による切り詰めが発生した場合は警告を出す。
 *
 * @param fd      設定対象のソケットファイルディスクリプタ
 * @param config  datalen / sndbuf / preload を参照する設定構造体
 */
static void set_socket_buff(int fd, t_ping_config *config) {
  size_t send = (size_t)(config->datalen + 8);
  send += ((send + 511) / 512) * (IPV4_HEADER_SIZE + 240);
  if (send > INT_MAX)
    error(1, "Buffer size too large: %zu\n", send);

  int sndbuf = config->sndbuf ? config->sndbuf : (int)send;
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0)
    error(1, "setsockopt SO_SNDBUF failed: %s\n", strerror(errno));

  int hold;
  if ((int)send > INT_MAX / config->preload) {
    error(0, "WARNING: buffer size overflow, reduce packet size or preload\n");
    hold = INT_MAX;
  } else {
    hold = (int)send * config->preload;
  }

  int rcvbuf = hold;
  if (hold < 65536)
    hold = 65536;

  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &hold, sizeof(hold));
  socklen_t tmplen = sizeof(hold);
  if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &hold, &tmplen) == 0) {
    if (hold < rcvbuf)
      error(0, "WARNING: probably, rcvbuf is not enough to hold preload\n");
  }
}

int ping_init(t_ping_session *session, char *target) {
  t_ping_net_state *net = &session->net;
  t_ping_config *config = &session->config;

  net->ident = config->ident ? config->ident : (uint16_t)(getpid() & 0xFFFF);

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

  set_socket_buff(fd, config);

  /* SO_SNDTIMEO: 送信が永久ブロックしないよう上限を 1 秒(または interval) に。 */
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (config->interval_ms < 1000) {
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * SCHINT(config->interval_ms);
  }
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  /* SO_RCVTIMEO: recvmsg 自体にタイムアウト能力を持たせて poll() を省ける。
   * iputils sock_setbufs (ping_common.c:548) と同じ最適化。失敗時は
   * opt_flood_poll を立てて poll 経路に強制する。 */
  tv.tv_sec = SCHINT(config->interval_ms) / 1000;
  tv.tv_usec = 1000 * (SCHINT(config->interval_ms) % 1000);
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
    config->opt_flood_poll = 1;

  net->socket_state.ops->extra_configure(fd);

  dns_lookup(target, &net->whereto);
  get_source_address(&net->from, &net->whereto, NULL);
  return 0;
}
