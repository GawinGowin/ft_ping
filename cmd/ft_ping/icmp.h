#ifndef ICMP_H
#define ICMP_H

#include <limits.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

/**
 * @brief SOCK_DGRAMソケット用のICMPパケット構造体
 * 
 * この構造体はIPヘッダーとICMPヘッダーを単一のパケット構造体に
 * 結合します。IPヘッダーとICMPヘッダーの両方を手動で構築する必要がある
 * SOCK_DGRAMソケットでの使用を想定して設計されています。
 * 
 * @note __attribute__((packed))により構造体メンバー間にパディングが
 *       追加されないことを保証し、ネットワーク送信に適した正しい
 *       パケット形式を維持します。
 * 
 * @warning この構造体はSOCK_DGRAMソケットでのみ使用します。
 *          SOCK_RAWソケットの場合、IPヘッダーは通常カーネルによって
 *          処理されます。
 */
typedef struct ip_icmp {
  struct iphdr ip;
  struct icmphdr icmp;
} t_ip_icmp __attribute__((packed));

#define GET_PACKET_DATA(packet)                                                                    \
  (packet + (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) * 3))

typedef struct s_icmp {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
  uint64_t timestamp;
} __attribute__((packed)) t_icmp; // メモリレイアウトの最適化を無効化

void generate_packet_data(void *packet, size_t datalen);
void set_timestamp(void *packet, size_t datalen, struct timeval *timestamp);
int create_echo_request_packet(void *packet, uint16_t id, uint16_t seq);
uint16_t calculate_checksum(void *data, int len);

#endif /* ICMP_H */
