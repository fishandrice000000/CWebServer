/*
 * tcp_fork_server.c — 多进程 TCP 网络服务 (V0.7)
 *
 * 父进程循环 accept, 每个客户端 fork 一个子进程处理 HTTP 请求.
 * 注册 SIGCHLD 处理函数回收子进程 (waitpid + WNOHANG).
 * accept 被信号打断时检查 EINTR 后继续等待.
 * 忽略 SIGPIPE 防止客户端异常断开时服务器崩溃.
 */

#include "tcp_fork_server.h"
#include "request_handler.h"
#include "log.h"
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

/* ---- SIGCHLD 处理: 循环 waitpid 回收所有已终止子进程 ---- */
static void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "tcp_fork_server: child %d terminated", pid);
        log_info(msg);
        printf("[server] child %d terminated\n", pid);
    }
}

int tcp_fork_server_run(const char *host, int port, UserNode *users,
                        int max_clients)
{
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    int client_count = 0;

    /* 1. socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return -1; }

    /* 允许端口快速重用 */
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (strcmp(host, "0.0.0.0") == 0 || strlen(host) == 0)
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "tcp_fork_server: invalid host %s\n", host);
        close(listen_fd);
        return -1;
    }

    if (bind(listen_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    /* 3. listen */
    if (listen(listen_fd, 5) < 0)
    {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    /* 注册信号处理 */
    signal(SIGCHLD, sig_chld);
    signal(SIGPIPE, SIG_IGN);

    {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "tcp_fork_server: listening on %s:%d (max_clients=%d)",
                 host, port, max_clients);
        log_info(msg);
    }
    printf("Server (fork) listening on %s:%d ...\n", host, port);

    /* 4. accept 循环 */
    while (1)
    {
        /* max_clients = 0 表示不限 */
        if (max_clients > 0 && client_count >= max_clients)
        {
            printf("[server] reached max_clients=%d, stopping accept.\n",
                   max_clients);
            break;
        }

        addr_len = sizeof(client_addr);
        conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr,
                         &addr_len);
        if (conn_fd < 0)
        {
            /* accept 可能被 SIGCHLD 打断 */
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }

        client_count++;

        {
            char msg[96];
            snprintf(msg, sizeof(msg),
                     "tcp_fork_server: client #%d from %s:%d",
                     client_count,
                     inet_ntoa(client_addr.sin_addr),
                     ntohs(client_addr.sin_port));
            log_info(msg);
        }

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
            close(listen_fd);               /* 子进程不需要监听 socket */

            handle_http_request(conn_fd, users);

            close(conn_fd);
            exit(0);
        }

        /* ===== 父进程 ===== */
        close(conn_fd);                     /* 父进程不需要已连接 socket */
    }

    /* 等待所有子进程结束 (可选: 避免服务器先于子进程退出) */
    while (waitpid(-1, NULL, 0) > 0)
        ;
    /* 恢复默认信号处理 */
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);

    log_info("tcp_fork_server: done");
    close(listen_fd);
    return 0;
}
