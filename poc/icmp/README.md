# ICMP構造体使用例 POC

このディレクトリには、ft_pingプロジェクトで使用されるICMP構造体の実際の使用例が含まれています。

## ファイル構成

- **icmp_dgram_example.c** - `struct icmphdr`を使用した非特権ICMPパケット送信例
- **icmp_raw_example.c** - `struct ip_icmp`を使用した特権ICMPパケット送信例  
- **Makefile** - ビルドとテスト用設定
- **README.md** - このファイル

## ビルド方法

```bash
# 全ての例をビルド
make all

# 非特権例のみビルド
make dgram

# 特権例のみビルド  
make raw

# ヘルプ表示
make help
```

## 実行方法

### 1. DGRAM例（非特権モード）

```bash
# ビルド
make dgram

# ローカルホストに対して実行
./icmp_dgram_example 127.0.0.1

# 外部ホストに対して実行
./icmp_dgram_example 8.8.8.8
```

### 2. RAW例（特権モード）

```bash
# ビルド
make raw

# root権限で実行（必須）
sudo ./icmp_raw_example 127.0.0.1

# 外部ホストに対して実行
sudo ./icmp_raw_example 8.8.8.8
```

## テスト実行

```bash
# 非特権テスト
make test

# 特権テスト（sudo必要）
sudo make test-root

# 外部ホストテスト
make test-external
```

## 構造体の違い

### struct icmphdr（DGRAM例）
- **特徴**: 標準POSIX構造体
- **権限**: 非特権ユーザーで実行可能
- **ソケット**: `SOCK_DGRAM`
- **利点**: シンプルで安全
- **用途**: 一般的なpingアプリケーション

### struct ip_icmp（RAW例）
- **特徴**: IPヘッダ + ICMPヘッダの複合構造体
- **権限**: root権限必須
- **ソケット**: `SOCK_RAW`
- **利点**: IPレベルの完全制御
- **用途**: ネットワーク診断ツール、特殊な実装

## ft_pingプロジェクトとの関連

これらの例は、ft_pingプロジェクトの以下の機能を参考にしています：

- `generate_packet_data()` - ペイロードデータ生成
- `calculate_checksum()` - チェックサム計算
- ICMPパケット構造の設計

## 実行結果例

### DGRAM例の出力
```
ICMPエコーリクエスト送信中...
  宛先: 127.0.0.1
  パケットサイズ: 64 バイト
  ICMP ID: 12345
  シーケンス: 1
✓ 64 バイト送信完了
応答待機中...
✓ 応答受信: 64 バイト from 127.0.0.1
  ICMP タイプ: 0
  ICMP コード: 0
  ICMP ID: 12345
  シーケンス: 1
  RTT: 0.123 ms
```

### RAW例の出力
```
ICMPエコーリクエスト送信中（RAWソケット）...
  宛先: 127.0.0.1
  パケットサイズ: 84 バイト（IPヘッダ含む）
  IP ID: 12345
  ICMP ID: 12345
  シーケンス: 1
  送信元: 192.168.1.100
✓ 84 バイト送信完了
応答待機中...
✓ 応答受信: 84 バイト from 127.0.0.1
  IP プロトコル: 1
  ICMP タイプ: 0
  ICMP コード: 0
  ICMP ID: 12345
  シーケンス: 1
  RTT: 0.156 ms
```

## 注意事項

1. **RAW例の実行にはroot権限が必要です**
2. **ファイアウォール設定によっては動作しない場合があります**
3. **一部のネットワーク環境ではICMPが制限されている場合があります**
4. **実際のft_pingプロジェクトでは、より高度なエラーハンドリングと機能が実装されています**

## トラブルシューティング

### "Operation not permitted"エラー
- RAW例の場合：`sudo`を使用してください
- DGRAM例の場合：システムのICMP設定を確認してください

### タイムアウト
- ネットワーク接続を確認してください
- ファイアウォール設定を確認してください
- 宛先ホストがICMPに応答するか確認してください