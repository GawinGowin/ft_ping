#ifndef FT_PING_H
#define FT_PING_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#ifndef SOL_SOCKET
#define SOL_SOCKET IPPROTO_IP
#endif

typedef struct ping_state {
  int sockfd;
  int datalen;
  long npackets;

  struct sockaddr_in whereto;

  char *hostname;
  unsigned int opt_verbose : 1;
} t_ping_state;

/* UseCases */
int parse_arg_usecase(int *argc, char ***argv, t_ping_state *state);
void show_usage_usecase(void);

/* lifecycle */
int init(t_ping_state *state, char **argv);
void cleanup(int status, void *state);

/* utils */
long parse_long(
    char const *const str,
    const char *const msg,
    const long min,
    const long max,
    void (*errorfn)(int, const char *, ...));

void error(int status, const char *format, ...);

#endif /* FT_PING_H */
