/*
 * ICMP RAW Socket Example  
 * struct ip_icmpを使用した特権ICMPパケット送信例
 * 注意: root権限が必要です
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

// ft_pingプロジェクトのip_icmp構造体定義を使用
struct ip_icmp {
  struct iphdr ip;
  struct icmphdr icmp;
};

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

// IPヘッダチェックサム計算
uint16_t ip_checksum(struct iphdr *ip_header) {
  ip_header->check = 0;
  return calculate_checksum(ip_header, ip_header->ihl * 4);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "使用法: %s <宛先IP>\n", argv[0]);
    return 1;
  }

  // root権限チェック
  if (getuid() != 0) {
    fprintf(stderr, "エラー: このプログラムはroot権限で実行してください\n");
    return 1;
  }

  const char *dest_ip = argv[1];
  const size_t payload_size = 56; // 標準的なpingペイロードサイズ
  const size_t packet_size = sizeof(struct ip_icmp) + payload_size;

  // パケットバッファの準備
  unsigned char packet_buffer[packet_size];
  memset(packet_buffer, 0, packet_size);

  struct ip_icmp *packet = (struct ip_icmp *)packet_buffer;
  void *payload = packet_buffer + sizeof(struct ip_icmp);

  // 送信元IPアドレスの取得（簡易版）
  char hostname[256];
  struct hostent *host_entry;
  gethostname(hostname, sizeof(hostname));
  host_entry = gethostbyname(hostname);
  if (!host_entry) {
    fprintf(stderr, "送信元IPアドレスの取得に失敗\n");
    return 1;
  }

  // IPヘッダの設定
  packet->ip.version = 4;
  packet->ip.ihl = 5; // ヘッダ長（5 * 4 = 20バイト）
  packet->ip.tos = 0;
  packet->ip.tot_len = htons(packet_size);
  packet->ip.id = htons(getpid());
  packet->ip.frag_off = 0;
  packet->ip.ttl = 64;
  packet->ip.protocol = IPPROTO_ICMP;
  packet->ip.check = 0; // 後で計算

  // 送信元IPアドレス設定
  memcpy(&packet->ip.saddr, host_entry->h_addr_list[0], 4);

  // 宛先IPアドレス設定
  if (inet_pton(AF_INET, dest_ip, &packet->ip.daddr) <= 0) {
    fprintf(stderr, "無効なIPアドレス: %s\n", dest_ip);
    return 1;
  }

  // ICMPヘッダの設定
  packet->icmp.type = ICMP_ECHO;
  packet->icmp.code = 0;
  packet->icmp.checksum = 0; // 一旦0にしてから計算
  packet->icmp.un.echo.id = htons(getpid());
  packet->icmp.un.echo.sequence = htons(1);

  // ペイロードデータの生成（ft_pingのgenerate_packet_dataを参考）
  generate_packet_data(payload, payload_size);

  // タイムスタンプをペイロードの先頭に設定
  struct timeval timestamp;
  gettimeofday(&timestamp, NULL);
  if (payload_size >= sizeof(timestamp)) {
    memcpy(payload, &timestamp, sizeof(timestamp));
  }

  // チェックサムの計算
  packet->ip.check = ip_checksum(&packet->ip);
  packet->icmp.checksum = calculate_checksum(&packet->icmp, sizeof(struct icmphdr) + payload_size);

  // RAWソケットの作成（特権必要）
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0) {
    perror("socket() failed - root権限が必要です");
    return 1;
  }

  // IP_HDRINCLオプションの設定（自前でIPヘッダを含める）
  int hdrincl = 1;
  if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(hdrincl)) < 0) {
    perror("setsockopt IP_HDRINCL failed");
    close(sockfd);
    return 1;
  }

  // 宛先アドレスの設定
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = packet->ip.daddr;

  // パケット送信
  printf("ICMPエコーリクエスト送信中（RAWソケット）...\n");
  printf("  宛先: %s\n", dest_ip);
  printf("  パケットサイズ: %zu バイト（IPヘッダ含む）\n", packet_size);
  printf("  IP ID: %d\n", ntohs(packet->ip.id));
  printf("  ICMP ID: %d\n", ntohs(packet->icmp.un.echo.id));
  printf("  シーケンス: %d\n", ntohs(packet->icmp.un.echo.sequence));

  char src_ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &packet->ip.saddr, src_ip_str, INET_ADDRSTRLEN);
  printf("  送信元: %s\n", src_ip_str);

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
    // RAWソケットの場合、IPヘッダも含まれる
    struct iphdr *reply_ip = (struct iphdr *)recv_buffer;
    struct icmphdr *reply_icmp = (struct icmphdr *)(recv_buffer + (reply_ip->ihl * 4));

    char reply_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &reply_ip->saddr, reply_ip_str, INET_ADDRSTRLEN);

    printf("✓ 応答受信: %zd バイト from %s\n", received, reply_ip_str);
    printf("  IP プロトコル: %d\n", reply_ip->protocol);
    printf("  ICMP タイプ: %d\n", reply_icmp->type);
    printf("  ICMP コード: %d\n", reply_icmp->code);

    if (reply_icmp->type == ICMP_ECHOREPLY) {
      printf("  ICMP ID: %d\n", ntohs(reply_icmp->un.echo.id));
      printf("  シーケンス: %d\n", ntohs(reply_icmp->un.echo.sequence));

      // RTT計算（簡易版）
      size_t icmp_header_size = reply_ip->ihl * 4 + sizeof(struct icmphdr);
      if (received >= (ssize_t)(icmp_header_size + sizeof(struct timeval))) {
        struct timeval *sent_time = (struct timeval *)(recv_buffer + icmp_header_size);
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