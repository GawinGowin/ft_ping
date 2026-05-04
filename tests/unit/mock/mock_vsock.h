#ifndef MOCK_VSOCK_H
#define MOCK_VSOCK_H

extern "C" {
#include "vsock/vsock.h"
}

/* テスト中の Mock_socket_ops 呼び出し回数と直近引数を観測するためのグローバル */
struct MockVsockState {
  int build_ipheader_calls;
  int extract_icmp_calls;
  int packet_size_calls;
  int extra_configure_calls;
  int set_ident_calls;

  /* 直近呼び出しの引数キャプチャ */
  size_t last_packet_size_arg;
  uint16_t last_seq;
  size_t last_datalen;
};

extern MockVsockState g_mock_vsock_state;

extern "C" {
extern t_ping_socket_ops Mock_socket_ops;
}

/* カウンタとキャプチャ値をゼロクリアする */
void mock_vsock_reset(void);

/* 与えられた socket_st に Mock_socket_ops と擬似 fd をセットする
 * （sendto 等は実 fd でないと失敗するので、そこまで進まないテスト専用） */
void mock_vsock_attach(t_socket_st *st);

#endif /* MOCK_VSOCK_H */
