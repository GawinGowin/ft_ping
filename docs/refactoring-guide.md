# ft_ping リファクタリングガイド

## Before → After: 全体像

```mermaid
graph TB
    subgraph "Before（フラット構成）"
        direction TB
        B1[ft_ping.c<br/>メイン + ループ]
        B2[usecases.c<br/>設定 + 統計 + 受信 + 初期化 + シグナル]
        B3[infra.c<br/>ソケット + DNS]
        B4[icmp.c<br/>パケット構築]
        B5[utils.c<br/>ユーティリティ]
        B1 --> B2
        B1 --> B3
        B2 --> B3
        B2 --> B4
    end

    subgraph "After（Hexagonal構成）"
        direction TB
        A1[main.c<br/>エントリーポイント]
        
        subgraph core["core/"]
            A2[types.h<br/>型定義]
            A3[config.c/h<br/>設定・引数パース]
            A4[stats.c/h<br/>統計計算]
            A5[icmp.c/h<br/>パケット構築・解析]
        end
        
        subgraph ports["ports/"]
            A6[ports.h<br/>IOインターフェース]
        end
        
        subgraph adapters["adapters/"]
            A7[network.c/h<br/>ソケット・DNS]
            A8[signal_handler.c/h<br/>シグナル]
        end
        
        subgraph app["app/"]
            A9[app.c/h<br/>メインループ・初期化]
        end
        
        subgraph utils["utils/"]
            A10[utils.c/h<br/>error・parse_long]
        end
        
        A1 --> A9
        A9 --> A6
        A6 -.->|実装| A7
        A9 --> A4
        A9 --> A5
    end

    style B2 fill:#ff6b6b,stroke:#333,color:#fff
    style core fill:#e8f4fd,stroke:#4a9eff
    style ports fill:#fff3e0,stroke:#ffa94a
    style adapters fill:#e8f5e9,stroke:#4caf50
    style app fill:#f3e5f5,stroke:#9c27b0
```

## 依存関係の方向

リファクタ後の依存関係は**一方向**で、コアは外部を知らない：

```mermaid
graph LR
    main[main.c] --> app[app/]
    main --> adapters[adapters/]
    app --> core[core/]
    app --> ports[ports/]
    adapters -.->|実装| ports
    core -.-> |型のみ| types[core/types.h]

    style core fill:#4a9eff,stroke:#333,color:#fff
    style ports fill:#ffa94a,stroke:#333
    style adapters fill:#4caf50,stroke:#333,color:#fff
    style app fill:#9c27b0,stroke:#333,color:#fff
    style main fill:#e8e8e8,stroke:#333
```

**ポイント：**
- `core/` は `adapters/` を知らない
- `adapters/` は `ports/` のインターフェースを実装する
- `app/` がポートを通じてアダプターを呼び出す
- `main.c` がすべてを組み立てる（Composition Root）

## 変更点の詳細

### 1. 構造体の分解

旧 `t_ping_master` は30フィールド超の巨大構造体でした。これを関心領域ごとに分解：

```mermaid
classDiagram
    class t_ping_master_old {
        +socket_state: t_socket_st
        +datalen: int
        +ttl: int
        +tos: int
        +npackets: long
        +interval: int
        +whereto: sockaddr_in
        +from: sockaddr_in
        +hostname: char*
        +ntransmitted: int
        +nreceived: int
        +tmin: long
        +tmax: long
        +tsum: double
        +opt_verbose: unsigned
        +opt_adaptive: unsigned
        ... 15+ more fields
    }

    class t_ping_master_new {
        +config: t_ping_config
        +stats: t_ping_stats
        +net: t_ping_net
        +timer: t_ping_timer
    }

    class t_ping_config {
        +datalen: int
        +ttl: int
        +tos: int
        +npackets: long
        +interval: int
        +opt_verbose: unsigned
        +opt_adaptive: unsigned
    }

    class t_ping_stats {
        +ntransmitted: int
        +nreceived: int
        +tmin: long
        +tmax: long
        +tsum: double
        +rcvd_tbl: rcvd_table
    }

    class t_ping_net {
        +socket_state: t_socket_st
        +whereto: sockaddr_in
        +from: sockaddr_in
        +hostname: char*
        +ident: uint16_t
    }

    class t_ping_timer {
        +prev_send_time: timeval
        +schedule_waittime: unsigned long
    }

    t_ping_master_new *-- t_ping_config
    t_ping_master_new *-- t_ping_stats
    t_ping_master_new *-- t_ping_net
    t_ping_master_new *-- t_ping_timer
```

**効果：**
- 各関数が**どの関心領域に触っているか**一目瞭然
- `stats_gather()` は `t_ping_stats` しか更新しない → 副作用の範囲が明確
- 構造体を引数に渡す粒度を細かくできる → テスタビリティ向上

### 2. ファイル分割（usecases.c の解体）

旧 `usecases.c`（約400行）は以下の**5つの責任**を混載していた：

```mermaid
graph TB
    subgraph "旧 usecases.c（400行）"
        R1[設定初期化]
        R2[引数パース]
        R3[シグナルハンドラ]
        R4[初期化・ソケット設定]
        R5[統計計算・表示]
        R6[パケット受信・解析]
        R7[終了スケジュール]
    end

    subgraph "分割後"
        R1 -->|移動| F1[core/config.c]
        R2 -->|移動| F1
        R3 -->|移動| F2[adapters/signal_handler.c]
        R4 -->|移動| F3[app/app.c]
        R5 -->|移動| F4[core/stats.c]
        R6 -->|移動| F3
        R7 -->|移動| F3
    end

    style R1 fill:#ff6b6b,stroke:#333,color:#fff
    style R2 fill:#ff6b6b,stroke:#333,color:#fff
    style R3 fill:#ff6b6b,stroke:#333,color:#fff
    style R4 fill:#ff6b6b,stroke:#333,color:#fff
    style R5 fill:#ff6b6b,stroke:#333,color:#fff
    style R6 fill:#ff6b6b,stroke:#333,color:#fff
    style R7 fill:#ff6b6b,stroke:#333,color:#fff
    style F1 fill:#4a9eff,stroke:#333,color:#fff
    style F2 fill:#4caf50,stroke:#333,color:#fff
    style F3 fill:#9c27b0,stroke:#333,color:#fff
    style F4 fill:#4a9eff,stroke:#333,color:#fff
```

### 3. IOポートの導入

テスト時にソケットやネットワークが不要になる仕組み：

```mermaid
sequenceDiagram
    participant Main as main.c
    participant IO as t_ping_io
    participant Net as network.c
    participant App as app.c
    participant Core as core/

    Main->>IO: network_adapter_new()
    Note over IO: 関数ポインタを設定
    Main->>App: app_initialize(master, argv, &io)
    App->>IO: io->create_socket()
    IO->>Net: net_create_socket()
    Net-->>IO: fd
    App->>IO: io->dns_lookup()
    IO->>Net: net_dns_lookup()
    
    Main->>App: app_main_loop(master, packet, size, &io)
    loop ping loop
        App->>Core: icmp_set_header_data()
        App->>IO: io->send_packet()
        IO->>Net: net_send_packet()
        App->>IO: io->recv_packet()
        IO->>Net: net_recv_packet()
        App->>Core: icmp_parse_reply()
        App->>Core: stats_gather()
    end
    App->>Core: stats_finish()
```

**テスト時の差し替え例：**

```c
// テスト用のモックアダプター
static int mock_send(void *pkt, size_t len, int fd, struct sockaddr_in *to) {
    (void)pkt; (void)len; (void)fd; (void)to;
    return len;  // 常に成功
}

static ssize_t mock_recv(int fd, struct msghdr *msg, int flags) {
    (void)fd; (void)flags;
    // 事前に用意したエコー応答パケットをmsgに詰める
    memcpy(msg->msg_iov[0].iov_base, &mock_reply_packet, sizeof(mock_reply_packet));
    return sizeof(mock_reply_packet);
}

t_ping_io mock_io = {
    .create_socket = mock_create_socket,
    .dns_lookup = mock_dns_lookup,
    .get_source_address = mock_get_source,
    .configure_socket_timeouts = mock_configure_timeouts,
    .send_packet = mock_send,
    .recv_packet = mock_recv,
};

// これでroot権限もネットワークも不要でテスト可能！
app_main_loop(&master, packet, size, &mock_io);
```

### 4. 命名の改善

| Before | After | 理由 |
|--------|-------|------|
| `A(tbl, bit)` | `BITMAP_WORD(tbl, bit)` | 意図が明確 |
| `B(bit)` | `BITMAP_MASK(bit)` | 意図が明確 |
| `SCHINT(a)` | `CLAMP_INTERVAL(a)` | 「最小値でクランプ」の意 |
| `__schedule_exit` | `schedule_exit_internal` | ダブルアンダースコアは予約済み識別子 |
| `*_usecase()` | ファイル名で表現 | `core/stats.c` の `stats_gather()` |
| `configure_state_usecase()` | `config_init_default()` | 動詞+目的語で明確 |

### 5. static変数の排除

旧 `pinger()` 関数のstatic変数をタイマー構造体に移行：

```mermaid
graph LR
    subgraph "Before"
        P1["pinger() 内の static 変数<br/>prev: timeval<br/>now: timeval"]
    end
    
    subgraph "After"
        P2["t_ping_timer（構造体メンバ）<br/>prev_send_time: timeval<br/>schedule_waittime: unsigned long"]
    end

    P1 -->|移行| P2

    style P1 fill:#ff6b6b,stroke:#333,color:#fff
    style P2 fill:#4caf50,stroke:#333,color:#fff
```

**効果：**
- テスト間でstatic変数がリセットされない問題を解消
- 状態の寿命が明確（`t_ping_master` のライフタイムに一致）

## ファイル一覧と責任

```
cmd/ft_ping/
├── main.c                      # Composition Root（組み立て + エントリーポイント）
├── ft_ping.h                   # アンブレラヘッダー + 互換マクロ
├── core/                       # 🔵 外部依存なしの純粋ロジック
│   ├── types.h                 #    全構造体の型定義
│   ├── config.c/h              #    デフォルト設定、引数パース、usage表示
│   ├── stats.c/h               #    統計蓄積、重複検出、最終統計表示
│   └── icmp.c/h                #    ICMPパケット構築、応答パース
├── ports/                      # 🟠 インターフェース定義
│   └── ports.h                 #    t_ping_io（関数ポインタテーブル）
├── adapters/                   # 🟢 外部世界との接続実装
│   ├── network.c/h             #    ソケット、DNS、パケット送受信
│   └── signal_handler.c/h      #    SIGINT/SIGALRM ハンドラ
├── app/                        # 🟣 オーケストレーション
│   └── app.c/h                 #    初期化、メインループ、終了処理
└── utils/                      # ⚪ 汎用ユーティリティ
    └── utils.c/h               #    error()、parse_long()
```

## データフロー

```mermaid
graph TB
    subgraph "起動フェーズ"
        S1[main.c] -->|1. 設定初期化| S2[config_init_default]
        S1 -->|2. 引数パース| S3[config_parse_args]
        S1 -->|3. IOアダプター構築| S4[network_adapter_new]
        S1 -->|4. ネットワーク初期化| S5[app_initialize]
        S5 -->|io->create_socket| S6[net_create_socket]
        S5 -->|io->dns_lookup| S7[net_dns_lookup]
    end

    subgraph "実行フェーズ"
        E1[app_main_loop] -->|送信| E2[send_ping]
        E2 -->|Core| E3[icmp_set_header_data]
        E2 -->|io->send_packet| E4[net_send_packet]
        E1 -->|受信| E5[receive_replies]
        E5 -->|io->recv_packet| E6[net_recv_packet]
        E5 -->|Core| E7[icmp_parse_reply]
        E7 -->|Core| E8[stats_gather]
    end

    subgraph "終了フェーズ"
        F1[stats_finish] -->|stdout| F2[統計表示]
        F3[app_cleanup] -->|close/free| F4[リソース解放]
    end

    S1 --> E1
    E1 --> F1
    E1 --> F3

    style S2 fill:#4a9eff,stroke:#333,color:#fff
    style S3 fill:#4a9eff,stroke:#333,color:#fff
    style E3 fill:#4a9eff,stroke:#333,color:#fff
    style E7 fill:#4a9eff,stroke:#333,color:#fff
    style E8 fill:#4a9eff,stroke:#333,color:#fff
    style F1 fill:#4a9eff,stroke:#333,color:#fff
    style S4 fill:#4caf50,stroke:#333,color:#fff
    style S6 fill:#4caf50,stroke:#333,color:#fff
    style S7 fill:#4caf50,stroke:#333,color:#fff
    style E4 fill:#4caf50,stroke:#333,color:#fff
    style E6 fill:#4caf50,stroke:#333,color:#fff
```

🔵 = Core（外部依存なし）　🟢 = Adapter（外部接続）

## 今後の拡張ポイント

1. **テスト強化**: `t_ping_io` にモックを差し込んで、メインループの統合テストを追加
2. **出力のPort化**: `printf` をPort経由にすれば、出力フォーマットの切り替え（JSON出力等）が容易に
3. **IPv6対応**: `t_ping_net` にアドレスファミリを追加し、IPv6用アダプターを実装
4. **設定ファイル対応**: 新しいDriving Adapterとして設定ファイルリーダーを追加
