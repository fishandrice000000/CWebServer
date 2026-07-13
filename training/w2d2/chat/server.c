/*
 * W2D2 训练：多客户端 TCP 文本聊天 — 服务器端
 *
 * 用法: ./server <port>
 *
 * 每 accept 一个客户端就 fork 子进程处理, 父进程继续 accept。
 * 注册 SIGCHLD 处理函数回收子进程, 避免僵尸进程。
 * accept 被信号打断时检查 EINTR 并继续等待。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

/* SIGCHLD 处理函数: 循环 waitpid 回收所有已终止的子进程 */
void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("[server] child %d terminated\n", pid);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buf[BUF_SIZE];

    /* 1. socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    /* 2. bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    /* 3. listen */
    if (listen(listen_fd, 5) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    /* 注册 SIGCHLD 处理函数, 回收僵尸子进程 */
    signal(SIGCHLD, sig_chld);

    printf("Server listening on port %d, waiting for connections...\n", port);

    /* 4. accept 循环 —— 每次连接 fork 子进程处理 */
    while (1)
    {
        addr_len = sizeof(client_addr);
        conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (conn_fd < 0)
        {
            /* accept 可能被 SIGCHLD 信号打断, 此时继续等待 */
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }

        printf("Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            close(conn_fd);
            continue;
        }

        if (pid == 0)
        {
            /* ===== 子进程 ===== */
            close(listen_fd);   /* 子进程不需要监听 socket */

            /* 聊天循环 */
            while (1)
            {
                memset(buf, 0, sizeof(buf));
                int n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0)
                {
                    printf("[child %d] Client disconnected.\n", getpid());
                    break;
                }

                /* 去换行 */
                char *nl = strchr(buf, '\n');
                if (nl) *nl = '\0';

                printf("[child %d] Got: %s\n", getpid(), buf);

                /* 检查退出 */
                if (strcmp(buf, "exit") == 0)
                {
                    printf("[child %d] Client requested exit.\n", getpid());
                    break;
                }

                /* 回复: 原样回射 */
                send(conn_fd, buf, strlen(buf), 0);
            }

            close(conn_fd);
            exit(0);            /* 子进程退出, 向父进程发送 SIGCHLD */
        }

        /* ===== 父进程 ===== */
        close(conn_fd);         /* 父进程不需要已连接 socket */
    }

    close(listen_fd);
    return 0;
}
