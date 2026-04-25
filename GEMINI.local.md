## 現在のタスク概要
CLIエントリーポイント `src/tool_main.c` を作成する。lib/ の公開APIのみを使用する薄いラッパー。

## 背景
curlの `src/tool_*` パターンに倣い、CLI固有のコードを `src/` に分離する（§3, §6）。
`src/` は `include/ft_ping/ft_ping.h` の公開APIのみを通じて lib/ にアクセスする。

## やること
- [ ] `src/tool_main.c` の作成
  - `main()` → 引数パース → `ftping_init()` → `ftping_run()` → `ftping_get_stats()` → 表示 → `ftping_cleanup()`

## 既存コードとの対応
| 新しいファイル | 移行元 |
|-------------|--------|
| `src/tool_main.c` | `cmd/ft_ping/ft_ping.c: main()`, `entrypoint()` の一部 |

## 依存方向の制約
```
src/tool_main.c
  → include/ft_ping/ft_ping.h（公開APIのみ）
  → lib/shared/shared_*（共有ユーティリティは直接使用可）
  ✗ lib/*.h（内部ヘッダーは参照しない）
```

## 完了条件
- `src/tool_main.c` が存在し、公開API経由でpingを実行できる
- `lib/` の内部ヘッダーを `#include` していない
- ビルドして実際にpingが動作する

## 依存
- #39 （公開API）
- #45 （ping_loop — `ftping_init`/`ftping_run` の実装）
- #48 （tool_getparam — 引数パース）

## 参考
- `docs/architecture-proposal.md` §4「依存関係の方向」, §6「CLIツールからの使用例」, §10「データフロー」
