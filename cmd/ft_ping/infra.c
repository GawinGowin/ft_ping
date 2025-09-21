#include "ft_ping.h"

int is_ipv6_address(const char *addr) {
  if (!addr) {
    return 0;
  }
  struct in6_addr ipv6_addr;
  return inet_pton(AF_INET6, addr, &ipv6_addr) == 1;
}

int create_socket(t_socket_st *socket_state) {
  socket_state->fd = -1;
  socket_state->socktype = -1;
  // SOCK_RAW を作るにはroot権限が必要なことがある
  socket_state->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (socket_state->fd >= 0) {
    socket_state->socktype = SOCK_RAW;
    return socket_state->fd;
  }
  // SOCK_DGRAM で作る
  if (errno == EPERM || errno == EACCES) {
    socket_state->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (socket_state->fd >= 0) {
      socket_state->socktype = SOCK_DGRAM;
      return socket_state->fd;
    }
  }
  return -1;
}

void dns_lookup(const char *hostname, struct sockaddr_in *addr) {
  struct addrinfo hints = {
      .ai_family = AF_INET,      /* Allow only IPv4 */
      .ai_socktype = SOCK_DGRAM, /* Datagram socket */
      .ai_flags = AI_PASSIVE,    /* For wildcard IP address */
      .ai_protocol = 0,          /* Any protocol */
      .ai_canonname = NULL,
      .ai_addr = NULL,
      .ai_next = NULL,
  };
  struct addrinfo *result;
  int ret = getaddrinfo(hostname, NULL, &hints, &result);
  if (ret != 0 || !result) {
    error(1, "getaddrinfo failed: %s", gai_strerror(ret));
  }

  struct sockaddr_in *addr_in = (struct sockaddr_in *)result->ai_addr;
  memcpy(addr, addr_in, sizeof(struct sockaddr_in));
  freeaddrinfo(result);
}

int send_packet(void *packet, size_t packet_size, int sockfd, struct sockaddr_in *whereto) {
  int cc = sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)whereto, sizeof(*whereto));
  if (cc < 0) {
    return -1;
  }
  return cc;
}

/**
 * @brief pingソケットのタイムアウトを設定する
 *
 * この関数は、指定された間隔に基づいてpingソケットのタイムアウト値を設定します。
 * ping操作の適切な動作を保証するために、送信と受信の両方のタイムアウトを設定します。
 * フラッドモードが有効な場合、それに応じてポーリング機構を調整します。
 *
 * @param sockfd 設定するソケットファイルディスクリプタ
 * @param interval pingパケット間の時間間隔（ミリ秒）
 * @param opt_flood_poll フラッドポーリングオプションフラグへのポインタ、フラッドモードがアクティブな場合更新される
 */
void configure_socket_timeouts(int sockfd, int interval, int *opt_flood_poll) {
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (interval < 1000) {
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * SCHINT(interval);
  }
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
  tv.tv_sec = SCHINT(interval) / 1000;
  tv.tv_usec = 1000 * (SCHINT(interval) % 1000);
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv))) {
    *opt_flood_poll = 1;
  }
}

struct sockaddr_in get_source_address(struct sockaddr_in *dest, const char *device) {
    struct sockaddr_in source = {0};
    int probe_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe_fd < 0) {
        error(2, "socket creation failed");
    }

    if (device) {
        if (setsockopt(probe_fd, SOL_SOCKET, SO_BINDTODEVICE,
                       device, strlen(device) + 1) == -1) {
            error(2, "SO_BINDTODEVICE failed");
        }
    }

    struct sockaddr_in dst = *dest;
    dst.sin_port = htons(1025);

    if (connect(probe_fd, (struct sockaddr *)&dst, sizeof(dst)) == -1) {
        error(2, "connect failed");
    }

    socklen_t alen = sizeof(source);
    if (getsockname(probe_fd, (struct sockaddr *)&source, &alen) == -1) {
        error(2, "getsockname failed");
    }
    source.sin_port = 0;
    close(probe_fd);
    return source;
}
