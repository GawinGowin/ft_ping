# Hexagonal Architecture（ヘキサゴナルアーキテクチャ）

## 概要

Hexagonal Architecture（別名: Ports and Adapters）は、2005年にAlistair Cockburnが提唱したソフトウェア設計パターンです。

**核心のアイデア：アプリケーションのコアロジックは、外部の技術的詳細を一切知らない。**

```mermaid
graph TB
    subgraph "外の世界（Driving Side）"
        CLI[CLI / テスト / 別プログラム]
    end

    subgraph "アプリケーション"
        DP[Driving Port<br/>入口の定義]
        Core[Core<br/>純粋なビジネスロジック]
        DRP[Driven Port<br/>出口の定義]
    end

    subgraph "外の世界（Driven Side）"
        Infra[DB / ネットワーク / ファイルシステム]
    end

    CLI -->|使う| DP
    DP --> Core
    Core --> DRP
    DRP -->|使う| Infra

    style Core fill:#4a9eff,stroke:#333,color:#fff
    style DP fill:#ffa94a,stroke:#333,color:#fff
    style DRP fill:#ffa94a,stroke:#333,color:#fff
```

## 3つの構成要素

### 1. Core（コア / ドメイン）

アプリケーションの**本質的なロジック**を持つ層。外部への依存がゼロ。

- OSもDBもネットワークも知らない
- 純粋な関数・データ構造だけで構成
- **テストが最も容易**

### 2. Port（ポート）

コアと外部の**接続口の定義**。インターフェースに相当する。

| 種類 | 方向 | 役割 | 例 |
|------|------|------|-----|
| Driving Port | 外 → コア | コアに何をさせるか | `config_parse_args()` |
| Driven Port | コア → 外 | コアが外部に何を頼むか | `t_ping_io.send_packet()` |

### 3. Adapter（アダプター）

ポートの**具体的な実装**。実際の外部リソースとやり取りする。

- 本番用アダプター：実際のソケット操作、DNS解決
- テスト用アダプター：モック実装

```mermaid
graph LR
    subgraph Driving Adapters
        A1[CLI引数パーサー]
        A2[テストハーネス]
    end

    subgraph Ports
        P1((Driving<br/>Port))
        P2((Driven<br/>Port))
    end

    subgraph Core
        C[ビジネスロジック]
    end

    subgraph Driven Adapters
        B1[ソケット実装]
        B2[モック実装]
    end

    A1 --> P1
    A2 --> P1
    P1 --> C
    C --> P2
    P2 --> B1
    P2 --> B2

    style C fill:#4a9eff,stroke:#333,color:#fff
    style P1 fill:#ffa94a,stroke:#333
    style P2 fill:#ffa94a,stroke:#333
```

## 従来のアーキテクチャとの比較

### レイヤードアーキテクチャ（3層）

```mermaid
graph TB
    UI[UI層] --> BL[ビジネスロジック層] --> DA[データアクセス層]
    
    style UI fill:#e8e8e8,stroke:#333
    style BL fill:#4a9eff,stroke:#333,color:#fff
    style DA fill:#e8e8e8,stroke:#333
```

**問題点：** ビジネスロジックがデータアクセス層に**依存**する。テスト時にDBやソケットが必要。

### Hexagonal Architecture

```mermaid
graph LR
    UI[UI層] -->|Port| BL[ビジネスロジック層]
    BL -->|Port| DA[データアクセス層]
    
    style UI fill:#e8e8e8,stroke:#333
    style BL fill:#4a9eff,stroke:#333,color:#fff
    style DA fill:#e8e8e8,stroke:#333
```

**利点：** ビジネスロジックは**何にも依存しない**。依存の矢印が逆転する（**依存性逆転の原則**）。

## Cでの実現方法

Cにはインターフェース（Javaの`interface`やGoの`interface`）がないが、**関数ポインタ**で同等の機能を実現できる。

```c
// ─── Port定義（インターフェース） ───
typedef struct ping_io {
    int (*send_packet)(void *packet, size_t len, int sockfd, struct sockaddr_in *dest);
    ssize_t (*recv_packet)(int sockfd, struct msghdr *msg, int flags);
    void (*dns_lookup)(const char *hostname, struct sockaddr_in *addr);
} t_ping_io;

// ─── Core（ポートだけに依存） ───
void ping_loop(t_ping_master *master, t_ping_io *io) {
    // io->send_packet() で送る。実体がソケットかモックか知らない
    // io->recv_packet() で受ける
}

// ─── 本番アダプター ───
t_ping_io real_io = {
    .send_packet = net_send_packet,    // 実際のsendto()
    .recv_packet = net_recv_packet,    // 実際のrecvmsg()
    .dns_lookup  = net_dns_lookup,     // 実際のgetaddrinfo()
};

// ─── テストアダプター ───
t_ping_io mock_io = {
    .send_packet = mock_send_always_ok,
    .recv_packet = mock_recv_echo_reply,
    .dns_lookup  = mock_resolve_localhost,
};
```

## CLIツールとの相性

CLIツールは以下の特徴を持つため、Hexagonalとの相性が良い：

| CLIの特徴 | Hexagonalの恩恵 |
|-----------|----------------|
| 入力元が多様（引数, stdin, 環境変数） | Driving Adapterで吸収 |
| 出力先が多様（stdout, ファイル, ネットワーク） | Driven Adapterで吸収 |
| テスト困難（root権限、ネットワーク必要） | モックアダプターで解決 |
| ライブラリとしても使いたい | Coreを切り出せば再利用可能 |

## 参考文献

- Alistair Cockburn, "Hexagonal Architecture" (2005)
  - https://alistair.cockburn.us/hexagonal-architecture/
- Robert C. Martin, "Clean Architecture" (2017)
- Netflix Tech Blog, "Ready for changes with Hexagonal Architecture"
  - https://netflixtechblog.com/ready-for-changes-with-hexagonal-architecture-b315ec967749
