# setjmp/longjmp サンプルコード

このディレクトリにはsetjmp/longjmpの使い方を示すサンプルコードが含まれています。

## ファイル一覧

- `basic_example.c` - setjmp/longjmpの基本的な使い方
- `error_handling.c` - エラーハンドリングでの使用例（TRY/CATCHマクロ）
- `Makefile` - ビルド用Makefile

## ビルドと実行

```bash
# 全てのサンプルをビルド
make

# 基本例の実行
make run-basic

# エラーハンドリング例の実行
make run-error

# クリーンアップ
make clean
```

## 重要なポイント

### setjmp/longjmpの基本
- `setjmp(buf)`: ジャンプポイントを設定、初回は0を返す
- `longjmp(buf, val)`: setjmpの場所に戻る、valが戻り値になる
- valが0の場合は自動的に1になる

### 注意事項
- ローカル変数は`volatile`をつけないと値が保証されない
- スタックフレームを超えたlongjmpは危険
- C++では例外処理を使うべき（デストラクタが呼ばれない）

### 実用例
- エラー処理（C言語での例外処理風の実装）
- パーサーでの構文エラー処理  
- シグナルハンドラからの復帰