#include "tool_output.h"

#include <arpa/inet.h>
#include <limits.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>

void tool_output_header(const t_ping_config *config, struct in_addr addr, uint16_t ident) {
  if (!config)
    return;
  printf(
      "PING %s (%s): %d data bytes", config->hostname ? config->hostname : "", inet_ntoa(addr),
      config->datalen);
  if (config->opt_verbose) {
    printf(", id 0x%x = %d\n", (int)ident, (int)ident);
  } else {
    printf("\n");
  }
}

void tool_output_reply(const t_ftping_reply *reply, void *ctx) {
  if (!reply)
    return;
  const t_ping_config *config = (const t_ping_config *)ctx;

  if (config && config->opt_ptimeofday) {
    printf(
        "[%lu.%06lu] ", (unsigned long)reply->recv_time.tv_sec,
        (unsigned long)reply->recv_time.tv_usec);
  }

  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &reply->from_addr, ip_str, sizeof(ip_str));

  printf("%d bytes from %s: icmp_seq=%u", reply->bytes, ip_str, reply->seq);

  if (reply->ttl > 0)
    printf(" ttl=%d", reply->ttl);

  if (reply->triptime_us >= 0)
    printf(" time=%.3f ms", reply->triptime_us / 1000.0);

  if (reply->is_duplicate)
    printf(" (DUP!)");

  printf("\n");
}

void tool_output_finish(const t_ftping_summary *s) {
  if (!s)
    return;

  printf("--- %s ping statistics ---\n", s->hostname ? s->hostname : "");

  printf("%d packets transmitted, %d packets received", s->ntransmitted, s->nreceived);

  if (s->nrepeats)
    printf(", +%ld duplicates", s->nrepeats);
  if (s->nchecksum)
    printf(", +%ld corrupted", s->nchecksum);
  if (s->nerrors)
    printf(", +%ld errors", s->nerrors);

  if (s->ntransmitted) {
    int loss = (int)(((double)(s->ntransmitted - s->nreceived) * 100.0) / s->ntransmitted);
    printf(", %d%% packet loss", loss);
  }

  printf("\n");

  if (s->nreceived && s->timing) {
    long total = s->nreceived + s->nrepeats;
    long avg_rtt = (long)(s->tsum / total);

    /* 分散計算（オーバーフロー対策）。lib/ping_stats.c 旧版と同じロジック。 */
    long long variance;
    if (s->tsum < INT_MAX)
      variance = (long long)((s->tsum2 - ((s->tsum * s->tsum) / total)) / total);
    else
      variance = (long long)((s->tsum2 / total) - ((double)avg_rtt * avg_rtt));

    /* ニュートン法で平方根 */
    double std_dev = 0.0;
    if (variance > 0) {
      double x = (double)variance;
      double prev;
      do {
        prev = x;
        x = (x + variance / x) / 2.0;
      } while (x < prev && (prev - x) > 0.001);
      std_dev = x;
    }

    printf(
        "round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", s->tmin / 1000.0, avg_rtt / 1000.0,
        s->tmax / 1000.0, std_dev / 1000.0);
  }
}

void tool_output_error(const t_ftping_error_event *ev, void *ctx) {
  if (!ev)
    return;
  const t_ping_config *config = (const t_ping_config *)ctx;
  if (!config || !config->opt_verbose)
    return;

  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &ev->from_addr, ip_str, sizeof(ip_str));

  const char *desc = "ICMP message";
  if (ev->icmp_type == ICMP_TIME_EXCEEDED)
    desc = "Time exceeded";
  else if (ev->icmp_type == ICMP_DEST_UNREACH)
    desc = "Destination Host Unreachable";

  if (ev->orig_seq_valid)
    printf("From %s: icmp_seq=%u %s\n", ip_str, ev->orig_seq, desc);
  else
    printf("From %s: %s\n", ip_str, desc);
}
