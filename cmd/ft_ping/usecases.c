#include "ft_ping.h"

static void set_socket_buff(t_ping_master *master);

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
  while ((ch = getopt(*argc, *argv, "hvt:Q:c:S:s:l:")) != EOF) {
    switch (ch) {
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

void cleanup_usecase(int status, void *master) {
  (void)status;
  t_ping_master *st = (t_ping_master *)master;
  close(st->sockfd);
}
