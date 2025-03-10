#ifndef FT_PING_H
#define FT_PING_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET IPPROTO_IP
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "icmp.h"

#define IPV4_HEADER_SIZE 20

typedef struct ping_state {
  int sockfd;
  int datalen;
  int ttl;
  int tos;
  int preload;

  long npackets;
  struct sockaddr_in whereto;

  size_t sndbuf;
  size_t rcvbuf;

  char *hostname;
  unsigned int opt_verbose : 1;
} t_ping_state;

/* UseCases */
int initialize_usecase(t_ping_state *state, char **argv);
int parse_arg_usecase(int *argc, char ***argv, t_ping_state *state);
int send_ping_usecase(void *packet, size_t packet_size, int sockfd, struct sockaddr_in *whereto);
void show_usage_usecase(void);
void cleanup_usecase(int status, void *state);

/* Infra */
int is_ipv6_address(const char *addr);
int create_socket_with_fallback(void);
void dns_lookup(const char *hostname, struct sockaddr_in *addr);

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
