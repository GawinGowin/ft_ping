#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

jmp_buf error_jump;

#define TRY do { if (setjmp(error_jump) == 0) {
#define CATCH } else {
#define END_TRY } } while(0)

#define THROW(code) longjmp(error_jump, (code))

// エラーコード定義
#define ERROR_NULL_POINTER 1
#define ERROR_OUT_OF_MEMORY 2
#define ERROR_INVALID_INPUT 3

void simulate_malloc_failure() {
    printf("メモリ割り当て処理...\n");
    // malloc失敗をシミュレート
    THROW(ERROR_OUT_OF_MEMORY);
}

void simulate_null_pointer() {
    printf("NULL ポインタアクセス...\n");
    THROW(ERROR_NULL_POINTER);
}

int divide_by_zero_check(int a, int b) {
    printf("除算処理: %d / %d\n", a, b);
    if (b == 0) {
        THROW(ERROR_INVALID_INPUT);
    }
    return a / b;
}

void nested_function_call() {
    printf("ネストした関数呼び出し\n");
    simulate_malloc_failure();
    printf("この行は実行されません\n");
}

int main() {
    printf("=== エラーハンドリングでのlongjmp使用例 ===\n\n");
    
    // 例1: メモリ割り当て失敗
    printf("--- テスト1: メモリ割り当て失敗 ---\n");
    TRY {
        simulate_malloc_failure();
        printf("成功\n");
    } CATCH {
        printf("エラーをキャッチしました: メモリ割り当て失敗\n");
    } END_TRY;
    
    // 例2: NULL ポインタアクセス
    printf("\n--- テスト2: NULL ポインタアクセス ---\n");
    TRY {
        simulate_null_pointer();
        printf("成功\n");
    } CATCH {
        printf("エラーをキャッチしました: NULL ポインタアクセス\n");
    } END_TRY;
    
    // 例3: 0除算
    printf("\n--- テスト3: 0除算 ---\n");
    TRY {
        int result = divide_by_zero_check(10, 0);
        printf("結果: %d\n", result);
    } CATCH {
        printf("エラーをキャッチしました: 不正な入力（0除算）\n");
    } END_TRY;
    
    // 例4: ネストした関数からのエラー
    printf("\n--- テスト4: ネストした関数からのエラー ---\n");
    TRY {
        nested_function_call();
        printf("成功\n");
    } CATCH {
        printf("エラーをキャッチしました: ネストした関数からのエラー\n");
    } END_TRY;
    
    // 例5: 正常なケース
    printf("\n--- テスト5: 正常なケース ---\n");
    TRY {
        int result = divide_by_zero_check(10, 2);
        printf("結果: %d\n", result);
        printf("正常に完了しました\n");
    } CATCH {
        printf("エラーが発生しました\n");
    } END_TRY;
    
    printf("\nプログラム正常終了\n");
    return 0;
}