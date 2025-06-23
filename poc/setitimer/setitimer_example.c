#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#define MILLI_SEC 1000

/* ユーザ定義関数の宣言 */
void SetSignal(int SignalName);
void SigHandler(int SignalName);

int main() {
  int use_cpu;
  struct itimerval value = {{0, 500 * MILLI_SEC}, {0, 500 * MILLI_SEC}};
  struct itimerval ovalue = {{0, 0}, {0, 0}};

  /* シグナルの設定 */
  SetSignal(SIGVTALRM);

  if (fork() == 0) {
    printf("子プロセス開始\n");

    if (setitimer(ITIMER_VIRTUAL, &value, &ovalue) == 0) {
      /* CPUを使う */
      for (use_cpu = 0; use_cpu < INT_MAX; ++use_cpu)
        ;
    }

    printf("\n子プロセス終了\n");
  } else {
    /* 親プロセス */
    wait(NULL);
    printf("親プロセス終了\n");
  }

  return 0;
}

/* シグナル受信/処理 */
void SigHandler(int p_signame) {
  printf("*");
  fflush(stdout);

  /* シグナルの再設定 */
  SetSignal(p_signame);
  return;
}

/* シグナルの設定 */
void SetSignal(int p_signame) {
  if (signal(p_signame, SigHandler) == SIG_ERR) {
    perror("SetSignal() ");
  }
  return;
}