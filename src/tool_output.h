#ifndef TOOL_OUTPUT_H
#define TOOL_OUTPUT_H

#include "ft_ping.h"

#include <netinet/in.h>
#include <stddef.h>

/* 開始時のヘッダー: "PING localhost (127.0.0.1): 56 data bytes" */
void tool_output_header(const t_ping_config *config, struct in_addr addr, uint16_t ident);

/* 1 応答ライン。ftping_set_reply_handler に登録するコールバック。
 * ctx は const t_ping_config * を渡す（-D 判定のため）。 */
void tool_output_reply(const t_ftping_reply *reply, void *ctx);

/* 終了時の統計出力 */
void tool_output_finish(const t_ftping_summary *summary);

/* -v 時のエラーパケット表示。ftping_set_error_handler に登録するコールバック。
 * ctx は const t_ping_config * を渡す（opt_verbose 判定のため）。 */
void tool_output_error(const t_ftping_error_event *ev, void *ctx);

#endif /* TOOL_OUTPUT_H */
