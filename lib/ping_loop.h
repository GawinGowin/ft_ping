#ifndef PING_LOOP_H
#define PING_LOOP_H

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "ping_config.h"
#include "ping_icmp.h"
#include "ping_schedule.h"
#include "ping_stats.h"
#include "shared/shared_error.h"
#include "shared/shared_net.h"
#include "vsock/vsock.h"

typedef struct ping_timer {
  struct timeval prev_send_time;
  unsigned long schedule_waittime;
} t_ping_timer;

typedef struct ping_net_state {
  t_socket_st socket_state;
  struct sockaddr_in whereto;
  struct sockaddr_in from;
  uint16_t ident;
} t_ping_net_state;

typedef struct ping_session {
  t_ping_config config;
  t_ping_stats_internal stats;
  t_ping_timer timer;
  t_ping_net_state net;
  volatile int is_exiting; /* ping_stop() がセット */
} t_ping_session;

typedef struct ping_receive {
  int *socket_fd;
  size_t packlen;
  int *polling;
  char addrbuf[128];
  char ans_data[4096];
  struct iovec *iov;
  struct msghdr *msg;
} t_ping_receive;

void ping_run(t_ping_session *session);
int ping_send_one(t_ping_session *session, void *packet, size_t packet_size);
int ping_init(t_ping_session *session, char *target);
int ping_receive_replies(t_ping_session *session, t_ping_receive *received);

#endif /* PING_LOOP_H */
