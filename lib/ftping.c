#include "ft_ping/ft_ping.h"
#include "ping_loop.h"
#include "ping_stats.h"
#include <stdlib.h>
#include <string.h>

t_ping_session *ftping_init(const t_ping_config *config, const char *target) {
    t_ping_session *session = malloc(sizeof(t_ping_session));
    if (!session) return NULL;
    memset(session, 0, sizeof(*session));
    session->config = *config;
    if (ping_init(session, (char *)target) < 0) {
        free(session);
        return NULL;
    }
    return session;
}

void ftping_run(t_ping_session *session) {
    ping_run(session);
}

t_ftping_stats ftping_get_stats(const t_ping_session *session) {
    t_ftping_stats out;
    memset(&out, 0, sizeof(out));
    if (!session) return out;
    out.ntransmitted = session->stats.ntransmitted;
    out.nreceived    = session->stats.nreceived;
    out.tmin         = session->stats.tmin;
    out.tmax         = session->stats.tmax;
    out.tsum         = session->stats.tsum;
    memcpy(&out.rcvd_tbl, &session->stats.rcvd_tbl, sizeof(out.rcvd_tbl));
    return out;
}

void ftping_stop(t_ping_session *session) {
    if (session) {
        session->is_exiting = 1;
    }
}

void ftping_cleanup(t_ping_session *session) {
    if (!session) return;
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
