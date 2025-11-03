#include "ft_ping.h"

static void set_socket_buff(t_ping_master *master);
static int __schedule_exit(t_ping_master *master, int next);

void configure_state_usecase(t_ping_master *master) {
  master->socket_state.fd = -1;
  master->socket_state.socktype = -1;
  master->datalen = 56;
  master->ttl = 64;
  master->tos = 0;
  master->preload = 1;
  master->sndbuf = 0;
  master->interval = 1000;
  master->npackets = 0;
  master->opt_adaptive = 0;
  master->opt_verbose = 0;
  master->opt_flood_poll = 0;
  master->deadline = 0;
  master->ntransmitted = 0;
  master->nreceived = 0;
  master->nrepeats = 0;
  master->nchecksum = 0;
  master->nerrors = 0;
  master->tmin = LONG_MAX;
  master->tmax = 0;
  master->tsum = 0.0;
  master->tsum2 = 0.0;
  master->rtt = 0;
  master->pipesize = 0;
  master->lingertime = 10;
}

static void signal_handler(int signo __attribute__((__unused__))) {
  global_state->is_exiting = 1;
  if (global_state->is_in_printing_addr) {
    longjmp(global_state->pr_addr_jmp, 0);
  }
}

void setup_signal_handlers_usecase(void) {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    error(1, "set signal SIGINT failed\n");
  }
  if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) { // status_snapshot: 実装するかは検討
    error(1, "set signal SIGQUIT failed\n");
  }
  if (signal(SIGALRM, signal_handler) == SIG_ERR) {
    error(1, "set signal SIGALRM failed\n");
  }
}

void show_usage_usecase(void) {
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

int initialize_usecase(t_ping_master *master, char **argv) {
  char *target = *argv;
  master->hostname = target;

  if (is_ipv6_address(target)) {
    error(2, "IPv6 is not supported\n");
  }

  // ソケット作成
  if (create_socket(&master->socket_state) < 0) {
    error(1, "Failed to create socket: %s\n", strerror(errno));
  }
  int fd = master->socket_state.fd;
  set_socket_buff(master);
  configure_socket_timeouts(fd, master->interval, &master->opt_flood_poll);
  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &master->ttl, sizeof(master->ttl)) < 0) {
    error(1, "setsockopt IP_TTL failed: %s\n", strerror(errno));
  }
  if (setsockopt(fd, IPPROTO_IP, IP_TOS, &master->tos, sizeof(master->tos)) < 0) {
    error(1, "setsockopt IP_TOS failed: %s\n", strerror(errno));
  }
  int on = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) < 0) {
    error(1, "setsockopt IP_RECVERR failed: %s\n", strerror(errno));
  }
  // IP_HDRINCLオプションの設定（自前でIPヘッダを含める）: これがないとRAWソケットで送信できない
  int hdrincl = 1;
  if (master->socket_state.socktype == SOCK_RAW &&
      setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(hdrincl)) < 0) {
    error(1, "setsockopt IP_HDRINCL failed: %s\n", strerror(errno));
  }
  dns_lookup(target, &master->whereto);
  get_source_address(&master->from, &master->whereto, NULL);
  return (0);
}

static void set_socket_buff(t_ping_master *master) {
  size_t send = master->datalen + 8;
  int fd = master->socket_state.fd;

  send += ((send + 511) / 512) * (IPV4_HEADER_SIZE + 240); // 240 is the overhead of IP options
  if (send > INT_MAX) {
    error(1, "Buffer size too large: %zu\n", send);
  }
  if (master->sndbuf == 0) {
    master->sndbuf = (int)send;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&master->sndbuf, sizeof(master->sndbuf)) < 0) {
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
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&master->rcvbuf, sizeof(master->rcvbuf));
  if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&hold, &tmplen) == 0) {
    if (hold < rcvbuf)
      error(0, "WARNING: probably, rcvbuf is not enough to hold preload\n");
  }
}

int parse_arg_usecase(int *argc, char ***argv, t_ping_master *master) {
  int ch;
  while ((ch = getopt(*argc, *argv, "AhvDe:t:Q:c:S:s:l:w:")) != EOF) {
    switch (ch) {
    case 'A':
      master->opt_adaptive = 1;
      break;
    case 'v':
      master->opt_verbose = 1;
      break;
    case 'D':
      master->opt_ptimeofday = 1;
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
    case 'e':
      master->ident = htons((uint16_t)parse_long(optarg, "invalid argument", 0, 0xFFFF, error));
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
    t_socket_st *sock_state,
    struct sockaddr_in *from,
    struct sockaddr_in *whereto,
    void *packet,
    size_t packet_size,
    int datalen,
    uint16_t seq,
    struct timeval *timestamp) {
  if (sock_state->socktype == SOCK_RAW) {
    set_ip_header(packet, from->sin_addr, whereto->sin_addr, datalen);
  }
  set_icmp_header_data(packet, sock_state->socktype, seq, datalen, timestamp);
  return send_packet(packet, packet_size, sock_state->fd, whereto);
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

/**
 * ICMP応答パケットを受信し処理するユースケース関数
 * 
 * ソケットバッファに複数のパケットがたまっている可能性があるため、無限ループでパケットを受信
 * Raw socketならば、ほかの端末からのパケットも受信する可能性がある。
 * 
 * この関数は以下の処理を行う：
 * - ICMPエコー応答パケットの受信
 * - 受信したパケットの検証とフィルタリング
 * - パケット内容の解析と統計情報の更新
 * - タイムアウト処理
 * 
 * ping4_parse_replyとgather_statisticsを複合化した関数の想定
 * - ipアドレスが期待する送信先か否かのチェックはこの関数の序盤: パケットを受信した後くらいに行う。
 * - そのあとにprintの処理を行う。
 * 
 * @return 成功時は0、エラー時は負の値を返す
 */
int receive_replies_usecase(t_receive_replies_dto *dto) {
  if (dto == NULL) {
    return (0);
  }
  ssize_t ret = 0;
  struct msghdr *msg;
  msg = dto->msg;
  while (1) {
    // TODO: RTTの計測で使用する
    // struct timeval *recv_timep = NULL;    
    // struct timeval recv_time;

    // TODO: 自分のパケットか判断する機構
    // int not_ours = 0; 
  
    dto->iov->iov_len = dto->packlen;
    memset(msg, 0, sizeof(*msg));
    msg->msg_name = &dto->addrbuf;
    msg->msg_namelen = sizeof(dto->addrbuf);
    msg->msg_iov = dto->iov;
    msg->msg_iovlen = 1;
    msg->msg_control = &dto->ans_data;
    msg->msg_controllen = sizeof(dto->ans_data);
    ret = recvmsg(*dto->socket_fd, msg, *dto->polling);

    // TODO: 受信したパケットの送信元の確認方法
    struct sockaddr_in *from = (struct sockaddr_in *)msg->msg_name;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from->sin_addr, ip_str, sizeof(ip_str));
    printf("Received packet from: %s\n", ip_str);
    *dto->polling = MSG_DONTWAIT;
    break;
  }
  return (ret);
}

// int analyse_packet_usecase() {// TODO: 後で実装
//   return (0);
// }

int finish_usecase() { // TODO: 後で実装
  return (0);
}

void cleanup_usecase(int status, void *global_state) {
  (void)status;
  if (global_state == NULL) {
    return;
  }
  t_ping_state *st = (t_ping_state *)global_state;
  if (st->socket_state != NULL && st->socket_state->fd >= 0) {
    close(st->socket_state->fd);
  }
  if (st->allocated_packet_addr != NULL) {
    free(st->allocated_packet_addr);
  }
}
