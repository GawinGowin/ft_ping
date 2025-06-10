#include "ft_ping.h"
#include "icmp.h"

static void main_loop(t_ping_master *master);
static int pinger(t_ping_master *master, long *ntransmitted, void *packet, size_t packet_size);
static void configure_state(t_ping_master *master);

volatile int g_is_exiting = 0;

static void signal_handler(int signo) {
  if (signo == SIGINT) {
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
  if (signal(SIGINT, signal_handler) == SIG_ERR || signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
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
      inet_ntoa(master->whereto.sin_addr), master->datalen,
      packet_size + IPV4_HEADER_SIZE);
  
  /* Allocate memory for packet buffer (including IP header for receiving) */
  void *packet = malloc(packet_size + sizeof(struct ip));
  if (!packet) {
    error(1, "malloc failed\n");
  }

  long ntransmitted = 0;

  /* Set socket to non-blocking mode */
  int sock_flags = fcntl(master->sockfd, F_GETFL, 0);
  fcntl(master->sockfd, F_SETFL, sock_flags | O_NONBLOCK);

  /* Configure poll for timeout handling */
  struct pollfd pfd;
  pfd.fd = master->sockfd;
  pfd.events = POLLIN;

  /* Send preload packets if specified */
  if (master->preload > 0) {
    int i;
    for (i = 0; i < master->preload && (!master->npackets || i < master->npackets); i++) {
      int ret = pinger(master, &ntransmitted, packet, packet_size);
      if (ret < 0) {
        fprintf(stderr, "pinger error during preload: %s\n", strerror(errno));
      }
    }
  }

  /* Main ping loop */
  while (!g_is_exiting && (master->npackets <= 0 || ntransmitted < master->npackets)) {
    int next_ping_ms = pinger(master, &ntransmitted, packet, packet_size);
    if (next_ping_ms < 0) {
      fprintf(stderr, "pinger error: %s\n", strerror(errno));
      continue;
    }
    
    /* Wait for response or timeout */
    pfd.revents = 0;
    int ret = poll(&pfd, 1, next_ping_ms);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "poll error: %s\n", strerror(errno));
      break;
    }
    
    /* Timeout occurred */
    if (ret == 0) {
      master->stats.timeout_count++;
      if (master->opt_verbose) {
        fprintf(stderr, "Request timeout for icmp_seq=%ld\n", ntransmitted - 1);
      }
      continue;
    }
    
    /* Process received packets */
    if (ret > 0 && (pfd.revents & POLLIN)) {
      int count = 0;
      while (receive_packet(master, packet, packet_size) > 0 && count < 100) {
        count++;  /* Limit to prevent endless loop */
      }
    }
  }
  
  /* Print statistics on exit */
  print_statistics(master);
  free(packet);
}

static int pinger(t_ping_master *master, long *ntransmitted, void *packet, size_t packet_size) {
  static struct timeval prev = {0, 0};
  static struct timeval now;
  long time_delta;

  uint16_t seq = (uint16_t)(*ntransmitted % UINT16_MAX);
  if (prev.tv_sec == 0 && prev.tv_usec == 0) {
    gettimeofday(&prev, NULL);
    int cc = send_ping_usecase(
        master->sockfd, &master->whereto, packet, packet_size, master->datalen, seq, &prev);
    if (cc < 0) {
      // Note: 本物のpingでは送信間隔の調整など複雑なエラーハンドリングが行われていた
      return -1;
    }
    master->stats.sent++;  /* Update sent packet counter */
    (*ntransmitted)++;
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
  master->stats.sent++;  /* Update sent packet counter */
  (*ntransmitted)++;
  return master->interval;
}

static void print_statistics(t_ping_master *master) {
  double loss = 0.0;
  double rtt_avg = 0.0;
  double rtt_mdev = 0.0;
  
  if (master->stats.sent > 0) {
    loss = 100.0 - ((double)master->stats.received / (double)master->stats.sent * 100.0);
  }
  
  if (master->stats.received > 0) {
    rtt_avg = master->stats.rtt_sum / master->stats.received;
    
    /* Standard deviation calculation */
    if (master->stats.received > 1) {
      rtt_mdev = sqrt(
        (master->stats.rtt_sum_sq - (master->stats.rtt_sum * master->stats.rtt_sum) / master->stats.received) /
        (master->stats.received - 1)
      );
    }
  }
  
  printf("\n--- %s ping statistics ---\n", master->hostname);
  printf("%ld packets transmitted, %ld received, ", 
         master->stats.sent, master->stats.received);
  
  if (master->stats.errors > 0) {
    printf("+%ld errors, ", master->stats.errors);
  }
  
  printf("%.1f%% packet loss, time %ldms\n", 
         loss, (long)(master->stats.rtt_sum));
  
  if (master->stats.received > 0) {
    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
           master->stats.rtt_min, rtt_avg, master->stats.rtt_max, rtt_mdev);
  }
}

static void configure_state(t_ping_master *master) {
  /* Basic settings */
  master->sockfd = -1;
  master->datalen = 56;
  master->ttl = 60;
  master->tos = 0;
  master->preload = 1;
  master->sndbuf = 0;
  master->interval = 1000;
  master->npackets = -1;
  master->opt_verbose = 0;
  master->timeout = 1000;  /* Default timeout: 1 second */
  master->pid = (uint16_t)getpid() & 0xFFFF;
  
  /* Initialize statistics */
  memset(&master->stats, 0, sizeof(master->stats));
  master->stats.rtt_min = -1;  /* Will be set on first received packet */
}

#ifndef TESTING
int main(int argc, char **argv) { return entrypoint(argc, argv); }
#endif
