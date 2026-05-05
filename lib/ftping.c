#include "ft_ping/ft_ping.h"
#include "ping_loop.h"
#include "ping_stats.h"
#include <stdlib.h>
#include <string.h>

t_ping_session *ftping_init(const t_ping_config *config, const char *target) {
  t_ping_session *session = malloc(sizeof(t_ping_session));
  if (!session)
    return NULL;
  memset(session, 0, sizeof(*session));
  session->config = *config;
  if (ping_init(session, (char *)target) < 0) {
    free(session);
    return NULL;
  }
  return session;
}

void ftping_set_reply_handler(t_ping_session *session, t_ftping_reply_cb cb, void *ctx) {
  if (!session)
    return;
  session->reply_cb = cb;
  session->reply_ctx = ctx;
}

void ftping_set_error_handler(t_ping_session *session, t_ftping_error_cb cb, void *ctx) {
  if (!session)
    return;
  session->error_cb = cb;
  session->error_ctx = ctx;
}

void ftping_run(t_ping_session *session) { ping_run(session); }

t_ftping_summary ftping_get_summary(const t_ping_session *session) {
  t_ftping_summary out;
  if (!session) {
    memset(&out, 0, sizeof(out));
    return out;
  }
  ping_stats_compute_summary(
      &session->stats, session->config.hostname, session->config.interval_ms, &out);
  return out;
}

size_t ftping_get_packet_size(const t_ping_session *session) {
  if (!session || !session->net.socket_state.ops)
    return 0;
  return session->net.socket_state.ops->packet_size(session->config.datalen);
}

struct in_addr ftping_get_target_addr(const t_ping_session *session) {
  if (!session) {
    struct in_addr zero = {0};
    return zero;
  }
  return session->net.whereto.sin_addr;
}

uint16_t ftping_get_ident(const t_ping_session *session) { return (session->net.ident); }

void ftping_stop(t_ping_session *session) {
  if (session) {
    session->is_exiting = 1;
  }
}

void ftping_cleanup(t_ping_session *session) {
  if (!session)
    return;
  if (session->net.socket_state.fd >= 0) {
    close(session->net.socket_state.fd);
    session->net.socket_state.fd = -1;
  }
  free(session);
}

void ftping_config_init(t_ping_config *config) {
  if (config) {
    ping_config_init(config);
  }
}
