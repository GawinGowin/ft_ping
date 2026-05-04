#ifndef TOOL_OUTPUT_H
#define TOOL_OUTPUT_H

#include "ft_ping.h"

#include <netinet/in.h>
#include <stddef.h>

/* 開始時のヘッダー: "PING %s (%s) %d(%zu) bytes of data." */
void tool_output_header(const t_ping_config *config, struct in_addr addr, size_t packet_size);

/* 1 応答ライン。ftping_set_reply_handler に登録するコールバック。
 * ctx は const t_ping_config * を渡す（-D 判定のため）。 */
void tool_output_reply(const t_ftping_reply *reply, void *ctx);

/* 終了時の統計出力 */
void tool_output_finish(const t_ftping_summary *summary);

#endif /* TOOL_OUTPUT_H */
