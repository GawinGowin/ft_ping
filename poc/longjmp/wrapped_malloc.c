#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

jmp_buf error_jump;

#define TRY do { if (setjmp(error_jump) == 0) {
#define CATCH } else {
#define END_TRY } } while(0)

#define THROW(code) longjmp(error_jump, (code))

#define ERROR_OUT_OF_MEMORY 1

static void *__malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    THROW(ERROR_OUT_OF_MEMORY);
  }
  return ptr;
}

int main() {
  void *ptr = NULL;
  TRY {
    ptr = __malloc(LONG_MAX);
    free(ptr);
  }
  CATCH { printf("error: memory allocation failed\n"); }
  END_TRY;
}
