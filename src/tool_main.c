#include "ft_ping.h"
#include "tool_cleanup.h"
#include "tool_getparam.h"
#include "tool_output.h"
#include "tool_signal.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  t_ping_config config;
  ftping_config_init(&config);

  int parse_err = tool_parse_args(&argc, &argv, &config);
  if (parse_err != 0) {
    if (parse_err == 2) {
      fprintf(stderr, "Usage error\n");
    }
    return parse_err;
  }

  if (argc < 1) {
    fprintf(stderr, "ft_ping: missing host operand\n");
    return 1;
  }

  config.hostname = argv[0];
  t_ping_session *session = ftping_init(&config, config.hostname);
  if (!session) {
    fprintf(stderr, "ft_ping: failed to initialize session\n");
    return 1;
  }
  tool_cleanup_register(session);
  tool_setup_signals(session);

  ftping_set_reply_handler(session, tool_output_reply, &config);

  tool_output_header(&config, ftping_get_target_addr(session), ftping_get_packet_size(session));

  ftping_run(session);

  t_ftping_summary summary = ftping_get_summary(session);
  tool_output_finish(&summary);

  tool_cleanup(session);
  return 0;
}
