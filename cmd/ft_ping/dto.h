#ifndef DTO_H
#define DTO_H

#include <stdint.h>
#include <unistd.h>
#include <sys/uio.h>  /* struct iovec */

typedef struct ping_master t_ping_master;

typedef struct receive_replies_dto {
  int *socket_fd;
  size_t packlen;
  int *polling;
  char addrbuf[128];
  char ans_data[4096];
  struct iovec *iov;
  struct msghdr *msg;
  t_ping_master *master;  /* 統計処理用 */
} t_receive_replies_dto;

#endif /* DTO_H */
