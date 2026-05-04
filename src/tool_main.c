#include "ft_ping.h"
#include "tool_cleanup.h"
#include "tool_getparam.h"
#include "tool_output.h"
#include "tool_signal.h"

int main(int argc, char **argv) {
  t_ping_config config;
  ftping_config_init(&config);

  tool_parse_args(&argc, &argv, &config);
  if (argc < 1) {
    error(1, "usage error: Destination address required\n");
  }
  config.hostname = argv[0];
  t_ping_session *session = ftping_init(&config, config.hostname);
  if (!session) {
    error(1, "failed to initialize session\n");
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
