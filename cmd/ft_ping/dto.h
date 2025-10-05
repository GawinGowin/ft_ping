#ifndef DTO_H
#define DTO_H

#include <stdint.h>
#include <unistd.h>

typedef struct receive_replies_dto {
  int *socket_fd;
  size_t packlen;
  int *polling;
  char addrbuf[128];
  char ans_data[4096];
  struct iovec *iov;
  struct msghdr *msg;
} t_receive_replies_dto;

// typedef struct receive_replies_dto {
//   uint16_t expected_cksum;
//   int expected_packet_ident;
//   // received_packets_statics *statics;
// } t_receive_replies_dto;

typedef struct finish_dto {

} t_finish_dto;

#endif /* DTO_H */
