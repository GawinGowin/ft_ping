#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

jmp_buf jump_buffer;

void dangerous_function() {
    printf("危険な処理を開始...\n");
    
    // 何らかの条件でエラーが発生
    int error_condition = 1;
    
    if (error_condition) {
        printf("エラーが発生！longjmpで復帰します\n");
        longjmp(jump_buffer, 1);  // setjmpの場所に戻る（戻り値1）
    }
    
    printf("この行は実行されません\n");
}

void another_function() {
    printf("別の関数からlongjmpを呼び出し\n");
    longjmp(jump_buffer, 42);  // 戻り値42で復帰
}

int main() {
    printf("=== setjmp/longjmp 基本例 ===\n\n");
    
    // setjmpでジャンプポイントを設定
    int result = setjmp(jump_buffer);
    
    if (result == 0) {
        // 初回実行時（setjmpの直接的な戻り値）
        printf("setjmpを設定しました\n");
        dangerous_function();
        printf("通常の処理が完了\n");
    } else {
        // longjmpによる復帰時
        printf("longjmpから復帰しました（戻り値: %d）\n", result);
    }
    
    printf("\n--- 別のテスト ---\n");
    
    // 新しいジャンプポイントを設定
    result = setjmp(jump_buffer);
    
    if (result == 0) {
        printf("新しいsetjmpポイントを設定\n");
        another_function();
    } else {
        printf("another_functionからの復帰（戻り値: %d）\n", result);
    }
    
    printf("プログラム終了\n");
    return 0;
}