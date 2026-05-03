[![C/C++ Test](https://github.com/GawinGowin/ft_ping/actions/workflows/run-test.yaml/badge.svg)](https://github.com/GawinGowin/ft_ping/actions/workflows/run-test.yaml)
[![CodeQL](https://github.com/GawinGowin/ft_ping/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/GawinGowin/ft_ping/actions/workflows/github-code-scanning/codeql)
[![codecov](https://codecov.io/gh/GawinGowin/ft_ping/graph/badge.svg?token=ApUfGXUe1u)](https://codecov.io/gh/GawinGowin/ft_ping)

<p align="center">
<picture>
 <source media="(prefers-color-scheme: dark)" srcset="./images/ft_ping_dark.png">
 <source media="(prefers-color-scheme: light)" srcset="./images/ft_ping_light.png">
 <img alt="ft_ping_banner" src="./images/ft_ping_dark.png">
</picture>
</p>

<p align="center">
<strong>A reimplementation of the ping command in C</strong><br>
Version: 5.1
</p>

# ft_ping

ft_pingは、標準的なpingコマンドのC言語による再実装プロジェクトです。ICMPエコーリクエストパケットを送信し、ネットワーク接続を測定する機能を提供します。

## 🚀 特徴

- **ICMP Echo Request送信**: 正確なICMPパケット生成と送信
- **IPv4サポート**: IPv4アドレスとホスト名解決
- **豊富なオプション**: TTL、TOS、パケットサイズなどの設定可能
- **権限対応**: 特権モード（RAWソケット）と非特権モード（DGRAMソケット）の自動切り替え
- **テスト完備**: Google Testによる包括的なユニットテスト
- **厳格なコード品質**: AddressSanitizer、コードカバレッジ、静的解析

## 📋 現在の実装状況

### ✅ 実装済み機能
- コマンドライン引数解析
- DNS名前解決
- ICMPパケット作成と送信
- ソケット作成とフォールバック機能
- タイマーベースのパケット送信間隔制御
- 各種ソケットオプション設定（TTL、TOS等）

### 🚧 開発中・未実装機能
- ICMPエコーリプライの受信と処理
- RTT（往復時間）計算と表示
- パケットロス統計
- 最終統計サマリー
- IPv6サポート

## 🛠️ ビルド方法

### Makefileを使用（直接コンパイル / CMake不使用）

```bash
# リリースビルド（ft_ping バイナリ）
make

# デバッグビルド（AddressSanitizer付き / ft_ping_debug）
make debug

# コードフォーマット
make fmt
```

### CMake経由（テスト・カバレッジ含む）

```bash
# CMake で全体をビルド
make build

# テスト実行（unit + tool）
make test

# コードカバレッジ
make cov
```

### CMakeを直接使用

```bash
mkdir build && cd build
cmake ..
make
```

## 🔧 使用方法

```bash
# 基本的な使用法
./ft_ping google.com

# オプション付き実行
./ft_ping -c 4 -t 64 -s 32 example.com
```

### サポートするオプション

| オプション | 説明 | デフォルト値 |
|----------|------|-------------|
| `-c <count>` | 送信するパケット数 | 無制限 |
| `-t <ttl>` | Time To Live (1-255) | 64 |
| `-Q <tclass>` | Type of Service/QoS (0-255) | 0 |
| `-s <size>` | データペイロードサイズ | 56 bytes |
| `-S <size>` | ソケット送信バッファサイズ | システム値 |
| `-l <preload>` | プリロードパケット数 (0-65536) | 1 |
| `-v` | 詳細出力モード | オフ |
| `-h` | ヘルプ表示 | - |

## 🏗️ アーキテクチャ

curl の設計を参考に、ライブラリ（`lib/`）と CLI ツール（`src/`）を分離した構成を採用しています。詳細は [`CLAUDE.md`](./CLAUDE.md) を参照。

```
ft_ping/
├── include/ft_ping/         # 公開API（ライブラリのインターフェース）
│   └── ft_ping.h
├── lib/                     # libftping 本体（CLIを知らない）
│   ├── ftping.c             #   公開APIのラッパー
│   ├── ping_config.c/h      #   設定初期化
│   ├── ping_loop.c/h        #   メイン ping ループ
│   ├── ping_stats.c/h       #   統計
│   ├── ping_icmp.c/h        #   ICMPパケット構築・解析
│   ├── ping_schedule.c/h    #   終了スケジューリング
│   ├── vsock/               #   ソケット種別の vtable 抽象化
│   │   ├── vsock.c/h        #     バックエンド選択
│   │   ├── vsock_raw.c      #     SOCK_RAW
│   │   └── vsock_dgram.c    #     SOCK_DGRAM
│   └── shared/              #   共有ユーティリティ
│       ├── shared_error.c/h
│       ├── shared_net.c/h
│       └── shared_parse.c/h
├── src/                     # CLIツール（lib/ の公開APIのみ使用）
│   ├── tool_main.c          #   エントリーポイント
│   ├── tool_getparam.c/h    #   引数パース
│   ├── tool_signal.c/h      #   シグナルハンドラ
│   ├── tool_output.c/h      #   表示フォーマット
│   └── tool_cleanup.c/h     #   リソース解放
└── tests/                   # unit / tool / integration
```

### 主要な型定義

- `t_ping_session`: セッション状態（opaque、`lib/ping_loop.h`）
- `t_ping_config`: 公開設定構造体
- `t_ftping_summary`: 最終統計サマリー
- `struct ping_socket_ops`: ソケット種別の vtable

## 🧪 テスト

包括的なユニットテストスイートを提供：

```bash
# テスト実行
make test

# カバレッジレポート生成
make cov
```

### テスト対象機能
- 引数解析の全オプション
- ICMPパケット作成とデータ生成
- DNS解決機能（エラーケース含む）
- ソケット作成とフォールバック機能
- IPv6アドレス検出
- ユーティリティ関数

## 📊 コード品質

- **C11標準準拠**: 厳格なコンパイラフラグ（`-Wall -Wextra -Werror`）
- **メモリ安全性**: AddressSanitizerによる実行時チェック
- **コードカバレッジ**: lcovによるカバレッジ測定
- **静的解析**: CodeQLによるセキュリティ分析
- **フォーマット**: clang-formatによるコードスタイル統一

## 🤝 開発に参加

1. このリポジトリをフォーク
2. 機能ブランチを作成 (`git checkout -b feature/amazing-feature`)
3. 変更をコミット (`git commit -m 'Add amazing feature'`)
4. ブランチをプッシュ (`git push origin feature/amazing-feature`)
5. プルリクエストを作成

## 📄 ライセンス

このプロジェクトはLICENSEファイルに記載されたライセンスの下で配布されています。

---

