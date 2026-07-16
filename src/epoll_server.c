/*
 * epoll_server.c — epoll HTTP 服务器 (V1.0)
 *
 * 使用 epoll LT 模式实现高并发 Web Server.
 * 主循环: epoll_wait → accept 新连接 / handle_http_request 处理请求.
 * 客户端处理完毕后 epoll_ctl DEL + close.
 */

#include "epoll_server.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 256

int epoll_server_run(const char *host, int port, UserNode *users,
                     int max_requests)
{
    int listen_fd, epfd;
    struct sockaddr_in server_addr;
    struct epoll_event ev, events[MAX_EVENTS];
    int request_count = 0;

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
        fprintf(stderr, "epoll_server: invalid host %s\n", host);
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

    /* 4. epoll_create */
    epfd = epoll_create(1);
    if (epfd < 0) { perror("epoll_create"); close(listen_fd); return -1; }

    /* 5. 注册 listen_fd */
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        perror("epoll_ctl: listen_fd");
        close(epfd);
        close(listen_fd);
        return -1;
    }

    {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "epoll_server: listening on %s:%d (max_requests=%d)",
                 host, port, max_requests);
        log_info(msg);
    }
    printf("Server (epoll) listening on %s:%d "
           "(max_requests=%d) ...\n", host, port, max_requests);

    /* 6. 主循环 */
    while (1)
    {
        if (max_requests > 0 && request_count >= max_requests)
        {
            printf("[server] reached max_requests=%d, stopping.\n",
                   max_requests);
            break;
        }

        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0)
        {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            if (fd == listen_fd)
            {
                /* ---- 新连接 ---- */
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int conn_fd = accept(listen_fd,
                                     (struct sockaddr *)&client_addr,
                                     &addr_len);
                if (conn_fd < 0)
                {
                    perror("accept");
                    continue;
                }

                ev.events  = EPOLLIN;
                ev.data.fd = conn_fd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev) < 0)
                {
                    perror("epoll_ctl: conn_fd");
                    close(conn_fd);
                    continue;
                }

                {
                    char msg[96];
                    snprintf(msg, sizeof(msg),
                             "epoll_server: client from %s:%d (fd=%d)",
                             inet_ntoa(client_addr.sin_addr),
                             ntohs(client_addr.sin_port), conn_fd);
                    log_info(msg);
                }
            }
            else
            {
                /* ---- HTTP 请求 ---- */
                request_count++;

                {
                    char msg[64];
                    snprintf(msg, sizeof(msg),
                             "epoll_server: request #%d on fd=%d",
                             request_count, fd);
                    log_info(msg);
                }

                handle_http_request(fd, users);

                /* 从 epoll 移除并关闭 */
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            }
        }
    }

    log_info("epoll_server: done");
    close(epfd);
    close(listen_fd);
    return 0;
}
