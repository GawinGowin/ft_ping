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

### Makefileを使用

```bash
# リリースビルド
make

# デバッグビルド（AddressSanitizer付き）
make debug

# テスト実行
make test

# コードカバレッジ
make cov

# コードフォーマット
make fmt
```

### CMakeを使用

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

ft_pingは層別アーキテクチャを採用し、関心事の分離を実現しています：

```
ft_ping.c        # メインエントリポイントとプログラムループ
├── usecases.c   # ビジネスロジック層（引数解析、ping送信）
├── infra.c      # インフラ層（ソケット作成、DNS解決）
├── icmp.c       # ICMPパケット処理
└── utils.c      # ユーティリティ関数
```

### 主要な型定義

- `t_ping_master`: メイン状態構造体（ソケット、設定、ターゲット情報）
- `t_icmp`: ICMPパケット構造体

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

**注意**: 現在のバージョンはICMPパケットの送信機能のみを実装しています。受信とレスポンス処理機能は開発中です。