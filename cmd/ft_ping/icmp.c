#include "icmp.h"

static uint16_t calculate_checksum(void *data, int len);

int set_ip_header(void *packet, struct in_addr src_addr, struct in_addr dest_addr, size_t datalen) {
  if (packet == NULL || datalen <= 0) {
    return -1;
  }
  t_ip_icmp *packet_icmp = (t_ip_icmp *)packet;

  packet_icmp->ip.version = 4;
  packet_icmp->ip.ihl = 5;
  packet_icmp->ip.tos = 0;
  packet_icmp->ip.tot_len = htons(sizeof(t_ip_icmp) + datalen);
  packet_icmp->ip.id = htons(getpid());
  packet_icmp->ip.frag_off = 0;
  packet_icmp->ip.ttl = 64;
  packet_icmp->ip.protocol = IPPROTO_ICMP;
  packet_icmp->ip.check = 0; // 後で計算
  memcpy(&packet_icmp->ip.saddr, &src_addr, sizeof(src_addr));
  memcpy(&packet_icmp->ip.daddr, &dest_addr, sizeof(dest_addr));

  packet_icmp->ip.check = calculate_checksum(&packet_icmp->ip, packet_icmp->ip.ihl * 4);
  return 0;
}

int set_icmp_header_data(void *packet, int socktype, uint16_t seq, size_t datalen, struct timeval *timestamp) {
  if (packet == NULL || datalen <= 0) {
    return -1;
  }
  if (socktype != SOCK_RAW && socktype != SOCK_DGRAM) {
    return -1;
  }
  unsigned char *payload;
  struct icmphdr *icmp_hdr = NULL;

  if (socktype == SOCK_RAW) {
    t_ip_icmp *raw_icmp_hdr = (t_ip_icmp *)packet;
    icmp_hdr = &raw_icmp_hdr->icmp;
    payload = (unsigned char *)packet + sizeof(t_ip_icmp);
  } else if (socktype == SOCK_DGRAM) {
    icmp_hdr = (struct icmphdr *)packet;
    payload = (unsigned char *)packet + sizeof(struct icmphdr);
  }
  icmp_hdr->type = ICMP_ECHO;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0; // 後で計算
  icmp_hdr->un.echo.id = htons(getpid());
  icmp_hdr->un.echo.sequence = htons(seq);
  for (size_t i = 0; i < datalen; i++) {
    payload[i] = (unsigned char)((size_t)i % UCHAR_MAX);
  }
  if (datalen >= sizeof(*timestamp)) {
    memcpy(payload, timestamp, sizeof(*timestamp));
  }
  size_t packet_size = sizeof(struct icmphdr) + datalen;
  icmp_hdr->checksum = calculate_checksum((void *)icmp_hdr, packet_size);
  return 0;
}

static uint16_t calculate_checksum(void *data, int len) {
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
