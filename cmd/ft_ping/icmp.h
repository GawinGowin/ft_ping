#ifndef ICMP_H
#define ICMP_H

#define _GNU_SOURCE

#include <limits.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define	MAXIPLEN	60
#define	MAXICMPLEN	76

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
} t_ip_icmp;

int set_ip_header(void *packet, struct in_addr src_addr, struct in_addr dest_addr, size_t datalen);
int set_icmp_header_data(void *packet, int socktype, uint16_t seq, size_t datalen, struct timeval *timestamp) {

#endif /* ICMP_H */
