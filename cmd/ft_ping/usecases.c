#include "ft_ping.h"

static void set_socket_buff(t_ping_master *master);
static int __schedule_exit(t_ping_master *master, int next);

void show_usage_usecase(void) {
  char usage_msg[] = {
      "\nUsage:\n  %s [options] <destination>\n\n"
      "Options:\n"
      "  <destination>      dns name or ip address\n"
      "  -A                 use adaptive ping\n"
      "  -c <count>         stop after <count> replies\n"
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

int initialize_usecase(t_ping_master *master, char **argv) {
  char *target = *argv;
  master->hostname = target;

  if (is_ipv6_address(target)) {
    error(2, "IPv6 is not supported\n");
  }

  // ソケット作成
  master->sockfd = create_socket_with_fallback();
  if (master->sockfd < 0) {
    error(1, "Failed to create socket: %s\n", strerror(errno));
  }
  set_socket_buff(master);
  if (setsockopt(master->sockfd, IPPROTO_IP, IP_TTL, &master->ttl, sizeof(master->ttl)) < 0) {
    error(1, "setsockopt IP_TTL failed: %s\n", strerror(errno));
  }
  if (setsockopt(master->sockfd, IPPROTO_IP, IP_TOS, &master->tos, sizeof(master->tos)) < 0) {
    error(1, "setsockopt IP_TOS failed: %s\n", strerror(errno));
  }
  int on = 1;
  if (setsockopt(master->sockfd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) < 0) {
    error(1, "setsockopt IP_RECVERR failed: %s\n", strerror(errno));
  }
  dns_lookup(target, &master->whereto);
  return (0);
}

static void set_socket_buff(t_ping_master *master) {
  size_t send = master->datalen + 8;

  send += ((send + 511) / 512) * (IPV4_HEADER_SIZE + 240); // 240 is the overhead of IP options
  if (send > INT_MAX) {
    error(1, "Buffer size too large: %zu\n", send);
  }
  if (master->sndbuf == 0) {
    master->sndbuf = (int)send;
  }
  if (setsockopt(
          master->sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&master->sndbuf, sizeof(master->sndbuf)) <
      0) {
    error(1, "setsockopt SO_SNDBUF failed: %s\n", strerror(errno));
  }

  size_t rcvbuf, hold;
  rcvbuf = hold = send * master->preload;
  if (hold < 65536)
    hold = 65536;
  if (rcvbuf > INT_MAX || hold > INT_MAX) {
    error(1, "Buffer size too large: %zu\n", rcvbuf);
  }
  socklen_t tmplen = sizeof(hold);
  master->rcvbuf = (int)rcvbuf;
  setsockopt(
      master->sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&master->rcvbuf, sizeof(master->rcvbuf));
  if (getsockopt(master->sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&hold, &tmplen) == 0) {
    if (hold < rcvbuf)
      error(0, "WARNING: probably, rcvbuf is not enough to hold preload\n");
  }
}

int parse_arg_usecase(int *argc, char ***argv, t_ping_master *master) {
  int ch;
  while ((ch = getopt(*argc, *argv, "Ahvt:Q:c:S:s:l:w:")) != EOF) {
    switch (ch) {
    case 'A':
      master->opt_adaptive = 1;
      break;
    case 'v':
      master->opt_verbose = 1;
      break;
    case 't':
      master->ttl = parse_long(optarg, "invalid argument", 1, 255, error);
      break;
    case 'Q':
      master->tos = parse_long(optarg, "invalid argument", 0, 255, error);
      break;
    case 'c':
      master->npackets = parse_long(optarg, "invalid argument", 0, LONG_MAX, error);
      break;
    case 'S':
      master->sndbuf = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 's':
      master->datalen = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    case 'l':
      master->preload = parse_long(optarg, "invalid argument", 0, 65536, error);
      break;
    case 'w':
      master->deadline = parse_long(optarg, "invalid argument", 0, INT_MAX, error);
      break;
    default:
      return (2);
    }
  }
  *argc -= optind;
  *argv += optind;
  return (0);
}

int send_ping_usecase(
    int sockfd,
    struct sockaddr_in *whereto,
    void *packet,
    size_t packet_size,
    int datalen,
    uint16_t seq,
    struct timeval *timestamp) {
  create_echo_request_packet(packet, 0, seq);
  generate_packet_data(packet, datalen);
  set_timestamp(packet, datalen, timestamp);
  t_icmp *icmp = (t_icmp *)packet;
  icmp->checksum = 0;
  icmp->checksum = calculate_checksum(packet, packet_size);
  return send_packet(packet, packet_size, sockfd, whereto);
}

int schedule_exit(t_ping_master *master, int next) {
  if (master->npackets && master->ntransmitted >= master->npackets && !master->deadline)
    next = __schedule_exit(master, next);
  return next;
}

/**
 * @brief SIGALRM信号による適応的ping終了処理のスケジューリング
 * 
 * 指定されたパケット数の送信完了後、最後の応答パケットを受信するための
 * 適切な待機時間を計算し、SIGALRMタイマーを設定する。
 * 静的変数により複数回呼び出し時の重複実行を防止する。
 * 
 * ## 待機時間決定ロジック
 * 
 * ### ケース1: 応答パケット受信済み (`master->nreceived > 0`)
 * - 基本待機時間 = `2 × 最大RTT (master->tmax)`
 * - 最小保証時間 = `1000 × ping間隔 (master->interval)`
 * - より長い方を採用してネットワーク遅延に対応
 * 
 * ### ケース2: 応答パケット未受信 (`master->nreceived == 0`)
 * - 待機時間 = `lingertime × 1000` (マイクロ秒単位)
 * - デフォルトの待機戦略を適用
 * 
 * ## タイマー制御メカニズム
 * - `setitimer(ITIMER_REAL, ...)` による実時間ベースのタイムアウト
 * - SIGALRMハンドラー経由でのグレースフル終了処理
 * - 静的変数 `waittime` による初回実行のみの制御
 * 
 * @param master ping処理のマスター構造体（統計情報とネットワーク状態を含む）
 * @param next 次回パケット送信までの時間（秒単位）
 * @return 調整された次回送信時間（秒単位）、または元の値
 * 
 * @note この関数は初回呼び出し時のみ実際の処理を実行し、
 *       2回目以降は早期リターンによる効率化を図る
 * @note 参考実装: iputils ping の __schedule_exit() 関数
 */
static int __schedule_exit(t_ping_master *master, int next) {
  static unsigned long waittime;
  struct itimerval it;

  if (waittime)
    return next;
  if (master->nreceived) {
    waittime = 2 * master->tmax;
    if (waittime < 1000 * (unsigned long)master->interval)
      waittime = 1000 * master->interval;
  } else {
    waittime = master->lingertime * 1000;
  }
  if (next < 0 || (unsigned long)next < waittime / 1000)
    next = waittime / 1000;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 0;
  it.it_value.tv_sec = waittime / 1000000;
  it.it_value.tv_usec = waittime % 1000000;
  setitimer(ITIMER_REAL, &it, NULL);
  return next;
}

void cleanup_usecase(int status, void *master) {
  (void)status;
  t_ping_master *st = (t_ping_master *)master;
  close(st->sockfd);
}
