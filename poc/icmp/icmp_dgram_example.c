/*
 * ICMP DGRAM Socket Example
 * struct icmphdrを使用した非特権ICMPパケット送信例
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

// ft_pingプロジェクトのgenerate_packet_data関数を参考にした実装
void generate_packet_data(void *packet_data, size_t datalen) {
  if (packet_data == NULL || datalen <= 0) {
    return;
  }
  unsigned char *data = (unsigned char *)packet_data;
  for (size_t i = 0; i < datalen; i++) {
    data[i] = (unsigned char)(i % 255);
  }
}

// ICMPチェックサム計算（ft_pingから参考）
uint16_t calculate_checksum(void *data, int len) {
  uint32_t sum = 0;
  uint16_t *ptr = data;

  while (len > 1) {
    sum += *ptr++;
    len -= 2;
  }
  if (len == 1) {
    sum += *(uint8_t *)ptr;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "使用法: %s <宛先IP>\n", argv[0]);
    return 1;
  }

  const char *dest_ip = argv[1];
  const size_t payload_size = 56; // 標準的なpingペイロードサイズ
  const size_t packet_size = sizeof(struct icmphdr) + payload_size;

  // パケットバッファの準備
  unsigned char packet_buffer[packet_size];
  memset(packet_buffer, 0, packet_size);

  struct icmphdr *icmp_header = (struct icmphdr *)packet_buffer;
  void *payload = packet_buffer + sizeof(struct icmphdr);

  // ICMPヘッダの設定
  icmp_header->type = ICMP_ECHO;
  icmp_header->code = 0;
  icmp_header->checksum = 0; // 一旦0にしてから計算
  icmp_header->un.echo.id = htons(getpid());
  icmp_header->un.echo.sequence = htons(1);

  // ペイロードデータの生成（ft_pingのgenerate_packet_dataを参考）
  generate_packet_data(payload, payload_size);

  // タイムスタンプをペイロードの先頭に設定
  struct timeval timestamp;
  gettimeofday(&timestamp, NULL);
  if (payload_size >= sizeof(timestamp)) {
    memcpy(payload, &timestamp, sizeof(timestamp));
  }

  // チェックサムの計算
  icmp_header->checksum = calculate_checksum(packet_buffer, packet_size);

  // DGRAMソケットの作成（非特権）
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (sockfd < 0) {
    perror("socket() failed");
    return 1;
  }

  // 宛先アドレスの設定
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
    fprintf(stderr, "無効なIPアドレス: %s\n", dest_ip);
    close(sockfd);
    return 1;
  }

  // パケット送信
  printf("ICMPエコーリクエスト送信中...\n");
  printf("  宛先: %s\n", dest_ip);
  printf("  パケットサイズ: %zu バイト\n", packet_size);
  printf("  ICMP ID: %d\n", ntohs(icmp_header->un.echo.id));
  printf("  シーケンス: %d\n", ntohs(icmp_header->un.echo.sequence));

  ssize_t sent = sendto(
      sockfd, packet_buffer, packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

  if (sent < 0) {
    perror("sendto() failed");
    close(sockfd);
    return 1;
  }

  printf("✓ %zd バイト送信完了\n", sent);

  // 応答の受信を試行（タイムアウト付き）
  struct timeval timeout = {2, 0}; // 2秒タイムアウト
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  unsigned char recv_buffer[1024];
  struct sockaddr_in reply_addr;
  socklen_t addr_len = sizeof(reply_addr);

  printf("応答待機中...\n");
  ssize_t received = recvfrom(
      sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&reply_addr, &addr_len);

  if (received > 0) {
    struct icmphdr *reply_icmp = (struct icmphdr *)recv_buffer;
    char reply_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &reply_addr.sin_addr, reply_ip, INET_ADDRSTRLEN);

    printf("✓ 応答受信: %zd バイト from %s\n", received, reply_ip);
    printf("  ICMP タイプ: %d\n", reply_icmp->type);
    printf("  ICMP コード: %d\n", reply_icmp->code);

    if (reply_icmp->type == ICMP_ECHOREPLY) {
      printf("  ICMP ID: %d\n", ntohs(reply_icmp->un.echo.id));
      printf("  シーケンス: %d\n", ntohs(reply_icmp->un.echo.sequence));

      // RTT計算（簡易版）
      if (received >= (ssize_t)(sizeof(struct icmphdr) + sizeof(struct timeval))) {
        struct timeval *sent_time = (struct timeval *)(recv_buffer + sizeof(struct icmphdr));
        struct timeval current_time;
        gettimeofday(&current_time, NULL);

        long rtt_usec = (current_time.tv_sec - sent_time->tv_sec) * 1000000 +
                        (current_time.tv_usec - sent_time->tv_usec);
        printf("  RTT: %.3f ms\n", rtt_usec / 1000.0);
      }
    }
  } else {
    printf("タイムアウト または エラー\n");
  }

  close(sockfd);
  return 0;
}