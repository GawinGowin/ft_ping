#ifndef FT_PING_H
#define FT_PING_H

#define _GNU_SOURCE

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
#include <sys/socket.h>

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

/* utils */
long parse_long(char const *const str, const char *const msg, const long min, const long max);
void error(int status, const char *format, ...);
#endif /* FT_PING_H */
