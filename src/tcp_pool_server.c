/*
 * tcp_pool_server.c — 线程池 TCP 网络服务 (V0.8)
 *
 * 主线程: socket → bind → listen → accept 循环 → 任务入队.
 * worker 线程: 从队列取 client_fd → handle_http_request → close.
 * 使用 thread_pool 管理 worker 生命周期和任务分发.
 */

#include "tcp_pool_server.h"
#include "thread_pool.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tcp_pool_server_run(const char *host, int port, UserNode *users,
                        int num_workers, int max_clients)
{
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    int client_count = 0;

    /* 1. socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return -1; }

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
        fprintf(stderr, "tcp_pool_server: invalid host %s\n", host);
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

    /* 忽略 SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    /* 4. 初始化线程池 */
    thread_pool_t pool;
    if (thread_pool_init(&pool, num_workers, users) != 0)
    {
        fprintf(stderr, "tcp_pool_server: failed to init thread pool\n");
        close(listen_fd);
        return -1;
    }

    {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "tcp_pool_server: listening on %s:%d "
                 "(workers=%d, max_clients=%d)",
                 host, port, num_workers, max_clients);
        log_info(msg);
    }
    printf("Server (pool) listening on %s:%d "
           "(workers=%d, max_clients=%d) ...\n",
           host, port, num_workers, max_clients);

    /* 5. accept 循环 */
    while (1)
    {
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
            perror("accept");
            break;
        }

        client_count++;

        {
            char msg[96];
            snprintf(msg, sizeof(msg),
                     "tcp_pool_server: client #%d from %s:%d",
                     client_count,
                     inet_ntoa(client_addr.sin_addr),
                     ntohs(client_addr.sin_port));
            log_info(msg);
        }

        /* 提交任务到线程池 */
        if (thread_pool_add_task(&pool, conn_fd) != 0)
        {
            /* 线程池已 shutdown, 关闭连接 */
            close(conn_fd);
            break;
        }
    }

    /* 6. 关闭线程池 */
    printf("[server] shutting down thread pool...\n");
    thread_pool_shutdown(&pool);
    thread_pool_destroy(&pool);

    log_info("tcp_pool_server: done");
    close(listen_fd);
    return 0;
}
