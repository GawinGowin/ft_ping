# C言語における vtable パターン

## 概要

C でオブジェクト指向的な多態性（ポリモーフィズム）を実現するパターン。
関数ポインタを持つ構造体をインターフェースとして使い、実装ごとにグローバルインスタンスを定義する。

curl の `vtls/` ディレクトリや Linux カーネルの `file_operations` が同じ構造を採用している。

---

## 構成要素

### 1. インターフェース定義（ヘッダー）

```c
// vsock.h
typedef struct ping_socket_ops {
  int             (*configure)(int fd);
  int             (*build_packet)(void *packet, uint16_t seq, size_t datalen, struct timeval *ts);
  struct icmphdr *(*extract_icmp)(void *packet, size_t packet_len, int *icmp_len_out);
  size_t          (*packet_size)(size_t datalen);
} t_ping_socket_ops;

// 実装インスタンスの extern 宣言
extern t_ping_socket_ops Ping_socket_raw_ops;
extern t_ping_socket_ops Ping_socket_dgram_ops;
```

### 2. 実装定義（各 .c ファイル）

```c
// vsock_raw.c
static int raw_configure(int fd) {
  int on = 1;
  return setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
}
// ...

t_ping_socket_ops Ping_socket_raw_ops = {
  .create       = raw_create,
  .configure    = raw_configure,
  .build_packet = raw_build_packet,
  .extract_icmp = raw_extract_icmp,
  .packet_size  = raw_packet_size,
};
```

```c
// vsock_dgram.c
t_ping_socket_ops Ping_socket_dgram_ops = {
  .create       = dgram_create,
  .configure    = dgram_configure,
  // ...
};
```

### 3. 利用側（使う側は ops ポインタ経由で呼ぶ）

```c
// vtable を選択してセット
ping_socket_select(&master->socket_state);

// 種別を意識せず呼び出せる
master->socket_state.ops->configure(fd);
master->socket_state.ops->build_packet(packet, seq, datalen, &ts);
```

---

## C++ との対応

| C++ | C（このパターン） |
|-----|-----------------|
| `class` / `interface` | `typedef struct ping_socket_ops` |
| 仮想関数テーブル (vtable) | 関数ポインタのメンバー |
| `new RawSocket()` | `Ping_socket_raw_ops` グローバルインスタンス |
| 派生クラスのインスタンス | `socket_state->ops = &Ping_socket_raw_ops` |
| 仮想関数呼び出し | `socket_state->ops->configure(fd)` |

---

## extern 宣言の意味

```c
extern t_ping_socket_ops Ping_socket_raw_ops;
```

- `extern` は「実体は別の .c ファイルにある」という宣言
- `Ping_socket_raw_ops` は関数ポインタのみを持ち状態を持たないため、グローバルに1つあれば十分（シングルトン相当）
- ヘッダーに `extern` 宣言を置くことで、どのファイルからでも参照できる

実体は `vsock_raw.c` の `t_ping_socket_ops Ping_socket_raw_ops = { ... }` で定義される。

---

## このパターンのメリット

- 呼び出し側に `if (socktype == SOCK_RAW)` の分岐が不要になる
- 実装を追加するとき（例: IPv6対応）はインスタンスを1つ追加するだけ
- テストで差し替えが容易（モック用の ops インスタンスを渡せる）
