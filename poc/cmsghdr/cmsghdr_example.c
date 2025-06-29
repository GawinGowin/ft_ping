#define _DEFAULT_SOURCE
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MESSAGE_TEXT "Hello, cmsghdr!"
#define CONTROL_BUFFER_SIZE 1024
#define PORT 12345

/* プロトタイプ宣言 */
void sender_process(void);
void receiver_process(void);
void print_cmsghdr_info(struct cmsghdr *cmsg);
void analyze_control_messages(struct msghdr *msg);

int main(void) {
    pid_t pid;
    int status;

    printf("=== struct cmsghdr サンプルプログラム ===\n");

    /* 子プロセスで受信側、親プロセスで送信側を実行 */
    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* 子プロセス: 受信側 */
        receiver_process();
    } else {
        /* 親プロセス: 送信側 */
        /* 少し待ってから送信開始（受信側の準備を待つ） */
        usleep(100000); /* 100ms */
        sender_process();
        
        /* 子プロセスの終了を待つ */
        wait(&status);
        printf("プログラム完了\n");
    }

    return 0;
}

void sender_process(void) {
    int sock;
    struct sockaddr_in addr;
    struct msghdr msg;
    struct iovec iov;
    char control_buffer[CONTROL_BUFFER_SIZE];
    
    printf("送信側プロセス開始（PID: %d）\n", getpid());

    /* UDPソケット作成 */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket creation failed");
        exit(1);
    }

    /* 送信先アドレス設定 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);

    /* メッセージ構造体設定 */
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    
    /* データ部分設定 */
    iov.iov_base = MESSAGE_TEXT;
    iov.iov_len = strlen(MESSAGE_TEXT);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    /* 制御メッセージ領域設定（今回は送信時に特別な制御メッセージは付加しない） */
    msg.msg_control = control_buffer;
    msg.msg_controllen = 0; /* 送信時は制御メッセージなし */

    printf("メッセージを送信: %s\n", MESSAGE_TEXT);

    /* メッセージ送信 */
    if (sendmsg(sock, &msg, 0) == -1) {
        perror("sendmsg failed");
        close(sock);
        exit(1);
    }

    printf("パケット送信完了\n");
    close(sock);
    printf("送信側プロセス終了\n");
}

void receiver_process(void) {
    int sock;
    struct sockaddr_in addr, sender_addr;
    struct msghdr msg;
    struct iovec iov;
    char buffer[1024];
    char control_buffer[CONTROL_BUFFER_SIZE];
    ssize_t received;
    int on = 1;
    socklen_t sender_len;

    printf("受信側プロセス開始（PID: %d）\n", getpid());

    /* UDPソケット作成 */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket creation failed");
        exit(1);
    }

    /* SO_TIMESTAMP オプションを有効化 */
    if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on)) == -1) {
        perror("setsockopt SO_TIMESTAMP failed");
        close(sock);
        exit(1);
    }

    /* 受信アドレス設定 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    /* ソケットをアドレスにバインド */
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind failed");
        close(sock);
        exit(1);
    }

    /* メッセージ受信準備 */
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sender_addr;
    msg.msg_namelen = sizeof(sender_addr);
    
    /* データバッファ設定 */
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer) - 1; /* null終端用に1バイト残す */
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    /* 制御メッセージバッファ設定 */
    msg.msg_control = control_buffer;
    msg.msg_controllen = sizeof(control_buffer);

    /* メッセージ受信 */
    received = recvmsg(sock, &msg, 0);
    if (received == -1) {
        perror("recvmsg failed");
        close(sock);
        exit(1);
    }

    /* 受信データをnull終端 */
    buffer[received] = '\0';
    printf("\n受信メッセージ: %s\n", buffer);

    /* 送信者情報表示 */
    sender_len = msg.msg_namelen;
    if (sender_len >= sizeof(struct sockaddr_in)) {
        printf("送信者: %s:%d\n", 
               inet_ntoa(sender_addr.sin_addr), 
               ntohs(sender_addr.sin_port));
    }

    /* 制御メッセージの解析 */
    analyze_control_messages(&msg);

    close(sock);
    printf("受信側プロセス終了\n");
}

void analyze_control_messages(struct msghdr *msg) {
    struct cmsghdr *cmsg;
    
    printf("制御メッセージを解析中...\n");

    /* すべての制御メッセージを走査 */
    for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
        print_cmsghdr_info(cmsg);

        /* SO_TIMESTAMP の場合、タイムスタンプを詳細表示 */
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
            if (cmsg->cmsg_len >= CMSG_LEN(sizeof(struct timeval))) {
                struct timeval *tv = (struct timeval *)CMSG_DATA(cmsg);
                printf("  → タイムスタンプ: %ld.%06ld 秒\n", 
                       (long)tv->tv_sec, (long)tv->tv_usec);
                
                /* 現在時刻との差分計算（簡易的なレイテンシ測定） */
                struct timeval now;
                gettimeofday(&now, NULL);
                long diff_usec = (now.tv_sec - tv->tv_sec) * 1000000 + 
                                (now.tv_usec - tv->tv_usec);
                printf("  → 処理遅延: %ld マイクロ秒\n", diff_usec);
            } else {
                printf("  → 警告: タイムスタンプデータのサイズが不正\n");
            }
        }
    }

    if (CMSG_FIRSTHDR(msg) == NULL) {
        printf("制御メッセージが見つかりませんでした\n");
    }
    printf("\n");
}

void print_cmsghdr_info(struct cmsghdr *cmsg) {
    printf("- レベル: %d", cmsg->cmsg_level);
    
    /* よく使われるプロトコルレベルの表示 */
    switch (cmsg->cmsg_level) {
        case SOL_SOCKET:
            printf(" (SOL_SOCKET)");
            break;
        case IPPROTO_IP:
            printf(" (IPPROTO_IP)");
            break;
        case IPPROTO_IPV6:
            printf(" (IPPROTO_IPV6)");
            break;
        default:
            printf(" (Unknown)");
            break;
    }
    printf("\n");

    printf("- タイプ: %d", cmsg->cmsg_type);
    
    /* SO_TIMESTAMPの場合の特別表示 */
    if (cmsg->cmsg_level == SOL_SOCKET) {
        switch (cmsg->cmsg_type) {
            case SO_TIMESTAMP:
                printf(" (SO_TIMESTAMP)");
                break;
            default:
                printf(" (Other socket option)");
                break;
        }
    }
    printf("\n");

    printf("- データサイズ: %lu bytes\n", 
           (unsigned long)(cmsg->cmsg_len - CMSG_LEN(0)));
    printf("- 総サイズ: %lu bytes\n", (unsigned long)cmsg->cmsg_len);
}