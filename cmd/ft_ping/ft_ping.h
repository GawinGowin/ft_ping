#ifndef FT_PING_H
#define FT_PING_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET IPPROTO_IP
#endif

#include "icmp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define IPV4_HEADER_SIZE 20
#define MIN_INTERVAL_MS 10
#define SCHINT(a) (((a) <= MIN_INTERVAL_MS) ? MIN_INTERVAL_MS : (a))

#ifndef HZ
#define HZ sysconf(_SC_CLK_TCK)
#endif

typedef struct ping_state {
  volatile int is_in_printing_addr;
  volatile unsigned int is_exiting : 1;
  jmp_buf pr_addr_jmp;
} t_ping_state;

extern t_ping_state *global_state;

typedef struct socket_st {
  int fd;
  int socktype;
} t_socket_st;

typedef struct ping_master {
  t_socket_st socket_state;
  int datalen;
  int ttl;
  int tos;
  int preload;

  long npackets;
  int interval;

  struct sockaddr_in whereto;
  struct sockaddr_in from;
  size_t sndbuf;
  size_t rcvbuf;
  int deadline;

  int ntransmitted;
  int nreceived;
  int tmax;
  int lingertime;
  char *hostname;
  unsigned int opt_verbose : 1;
  unsigned int opt_adaptive : 1;
  int opt_flood_poll;
} t_ping_master;

/* UseCases */
void configure_state_usecase(t_ping_master *master);
void setup_signal_handlers_usecase(void);
int initialize_usecase(t_ping_master *state, char **argv);
int parse_arg_usecase(int *argc, char ***argv, t_ping_master *state);
void show_usage_usecase(void);

int send_ping_usecase(
    t_socket_st *sock_state,
    struct sockaddr_in *from,
    struct sockaddr_in *whereto,
    void *packet,
    size_t packet_size,
    int datalen,
    uint16_t seq,
    struct timeval *timestamp);

int schedule_exit(t_ping_master *master, int next);
void cleanup_usecase(int status, void *state);
int receive_replies_usecase(
    t_ping_master *master, void *packet_buffer, size_t packlen, int *polling, int *recv_error);

/* Infra */
int is_ipv6_address(const char *addr);
int create_socket(t_socket_st *socket_state);
void dns_lookup(const char *hostname, struct sockaddr_in *addr);
int send_packet(void *packet, size_t packet_size, int sockfd, struct sockaddr_in *whereto);
void configure_socket_timeouts(int sockfd, int interval, int *opt_flood_poll);
void get_source_address(struct sockaddr_in *src, struct sockaddr_in *dest, const char *device);

/* utils */
long parse_long(
    char const *const str,
    const char *const msg,
    const long min,
    const long max,
    void (*errorfn)(int, const char *, ...));

#ifdef TESTING
#include <setjmp.h>
extern int last_error_status;
extern char last_error_message[256];
extern jmp_buf test_err_jmp_buf;
extern int test_err_jmp_buf_set;
#endif

void error(int status, const char *format, ...);

#endif /* FT_PING_H */
