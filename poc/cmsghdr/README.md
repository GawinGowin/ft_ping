# struct cmsghdr サンプルプログラム

## 概要

このプログラムは、Linux の `struct cmsghdr` を使用してパケットの補助データ（ancillary data）を操作するサンプルです。UDP ソケットを使用してタイムスタンプやその他の制御情報を送受信する方法をデモンストレーションします。

## ファイル構成

- `cmsghdr_example.c` - メインプログラム
- `Makefile` - ビルド設定
- `README.md` - このドキュメント

## プログラムの動作

### 主な機能

1. **タイムスタンプ取得**
   - `SO_TIMESTAMP` ソケットオプションを使用
   - カーネルによる自動タイムスタンプ付加
   - `CMSG_FIRSTHDR()` / `CMSG_NXTHDR()` による制御メッセージの走査

2. **制御メッセージ操作**
   - `struct cmsghdr` を使用した補助データの読み書き
   - `CMSG_DATA()` マクロによるデータ部分へのアクセス
   - `CMSG_LEN()` / `CMSG_SPACE()` によるサイズ計算

3. **パケット送受信**
   - `sendmsg()` / `recvmsg()` システムコールの使用
   - `struct msghdr` による複雑なメッセージ構造の処理
   - UDP ソケットでのローカル通信

### プログラムフロー

1. UDP ソケットの作成と設定
2. `SO_TIMESTAMP` オプションの有効化
3. `fork()` で送信側と受信側のプロセスを分離
4. 送信側：
   - メッセージの送信
   - 制御メッセージの付加（オプション）
5. 受信側：
   - `recvmsg()` でメッセージ受信
   - 制御メッセージの解析
   - タイムスタンプの抽出と表示

## ビルドと実行

```bash
# ビルド
make

# 実行
./cmsghdr_example.out
```

## 期待される出力

```
=== struct cmsghdr サンプルプログラム ===
受信側プロセス開始（PID: XXXX）
送信側プロセス開始（PID: YYYY）
メッセージを送信: Hello, cmsghdr!
パケット送信完了

受信メッセージ: Hello, cmsghdr!
制御メッセージを解析中...
- レベル: 1 (SOL_SOCKET)
- タイプ: 29 (SO_TIMESTAMP)
- データサイズ: 16 bytes
- タイムスタンプ: 1640995200.123456 秒

受信側プロセス終了
送信側プロセス終了
プログラム完了
```

## 技術的な詳細

### struct cmsghdr の構造

```c
struct cmsghdr {
    socklen_t cmsg_len;    // データ長（ヘッダ含む）
    int       cmsg_level;  // プロトコルレベル
    int       cmsg_type;   // 制御メッセージタイプ
    // unsigned char cmsg_data[]; データ部分
};
```

### 主要なマクロ

```c
CMSG_FIRSTHDR(msg)       // 最初の制御メッセージ取得
CMSG_NXTHDR(msg, cmsg)   // 次の制御メッセージ取得
CMSG_DATA(cmsg)          // データ部分へのポインタ
CMSG_LEN(len)            // 必要なヘッダサイズ計算
CMSG_SPACE(len)          // アライメント考慮サイズ計算
```

### タイムスタンプ処理

```c
// SO_TIMESTAMP の有効化
int on = 1;
setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on));

// 制御メッセージからタイムスタンプ抽出
for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
        struct timeval *tv = (struct timeval *)CMSG_DATA(cmsg);
        printf("タイムスタンプ: %ld.%06ld\\n", tv->tv_sec, tv->tv_usec);
    }
}
```

## 学習ポイント

1. **補助データの活用**: パケットにメタデータを付加する方法
2. **精密なタイミング**: カーネルレベルでのタイムスタンプ取得
3. **システムコール**: `sendmsg()` / `recvmsg()` の高度な使用法
4. **メモリレイアウト**: 制御メッセージのメモリ構造理解

## セキュリティ考慮事項

- 制御メッセージのサイズ検証は重要（バッファオーバーフロー防止）
- `cmsg_len` の妥当性チェック
- ネットワークから受信したデータの検証

## 関連する `ft_ping` プロジェクトへの応用

このサンプルは `ft_ping` プロジェクトでの以下の機能実装に参考になります：

- ICMP パケット受信時のタイムスタンプ取得
- RTT 計算の精度向上
- パケットの詳細情報取得（TTL、ルーティング情報など）
- エラー情報の詳細取得

## 制限事項

- Linux 固有の機能を使用（`SO_TIMESTAMP`）
- UDP ソケットでのサンプル（ICMP での実装は別途考慮が必要）
- ローカル通信のみ（実際のネットワーク通信は未対応）

## 参考資料

- `man 7 socket` - ソケットオプションの詳細
- `man 3 cmsg` - 制御メッセージマクロの使用法
- `man 2 sendmsg` / `man 2 recvmsg` - システムコールの詳細