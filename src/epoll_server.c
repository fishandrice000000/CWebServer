/*
 * epoll_server.c — epoll HTTP 服务器 (W3D1)
 *
 * 使用 epoll LT 模式 + 完整 HTTP 协议解析.
 * 每个客户端维护独立接收缓冲区, 判断 \r\n\r\n 完整性后解析请求.
 * 路由: GET /, GET /hello, GET /users/<name>, POST /echo, 404, 405.
 * 记录系统日志和访问日志.
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

#define MAX_EVENTS  256
#define MAX_CLIENTS 256
#define HTTP_BUF_SZ 8192

typedef struct {
    int  fd;
    int  active;
    char ip[INET_ADDRSTRLEN];
    int  port;
    char buf[HTTP_BUF_SZ];
    int  buf_len;
} http_client_t;

static http_client_t clients[MAX_CLIENTS];

/* ---- 处理一个客户端的完整 HTTP 请求 ---- */
static void process_http_request(int idx, int epfd, UserNode *users,
                                      const char *doc_root,
                                      const route_config_t *routes,
                                      int route_count)
{
    http_client_t *c = &clients[idx];
    http_request_t req;

    if (http_parse_request(c->buf, c->buf_len, &req) != 0)
    {
        log_warning("http_parse_request: bad request");
        dprintf(c->fd,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n");
        access_log_write(c->ip, "-", "-", "-", 400, 0, "-");
        goto cleanup;
    }

    {
        char log_msg[320];
        snprintf(log_msg, sizeof(log_msg),
                 "request: %s %s %s", req.method, req.path, req.version);
        log_info(log_msg);
    }

    int sc = 200;
    int resp_bytes = http_handle_request(c->fd, &req, users, &sc,
                                         doc_root, routes, route_count);

    /* 访问日志 */
    {
        const char *ua = http_get_header(&req, "User-Agent");
        access_log_write(c->ip, req.method, req.path, req.version,
                         sc, resp_bytes > 0 ? resp_bytes : 0,
                         ua ? ua : "-");
    }

cleanup:
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
    c->active = 0;
    c->fd = -1;
    c->buf_len = 0;
}

int epoll_server_run(const char *host, int port, UserNode *users,
                     int max_requests, const char *doc_root,
                     const route_config_t *routes, int route_count)
{
    int listen_fd, epfd;
    struct sockaddr_in server_addr;
    struct epoll_event ev, events[MAX_EVENTS];
    int request_count = 0;

    memset(clients, 0, sizeof(clients));
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

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
                 "epoll_server: listening on %s:%d", host, port);
        log_info(msg);
    }
    printf("Server (epoll) listening on %s:%d ...\n", host, port);

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

                int slot = -1;
                for (int j = 0; j < MAX_CLIENTS; j++)
                {
                    if (!clients[j].active)
                    { slot = j; break; }
                }
                if (slot < 0)
                {
                    fprintf(stderr, "max clients reached.\n");
                    close(conn_fd);
                }
                else
                {
                    clients[slot].fd      = conn_fd;
                    clients[slot].active  = 1;
                    clients[slot].buf_len = 0;
                    strncpy(clients[slot].ip,
                            inet_ntoa(client_addr.sin_addr),
                            sizeof(clients[slot].ip) - 1);
                    clients[slot].port =
                        ntohs(client_addr.sin_port);

                    ev.events  = EPOLLIN;
                    ev.data.fd = conn_fd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);

                    {
                        char msg[96];
                        snprintf(msg, sizeof(msg),
                                 "client %s:%d connected",
                                 clients[slot].ip, clients[slot].port);
                        log_info(msg);
                    }
                }
            }
            else
            {
                /* ---- 客户端数据 ---- */
                int slot = -1;
                for (int j = 0; j < MAX_CLIENTS; j++)
                {
                    if (clients[j].active && clients[j].fd == fd)
                    { slot = j; break; }
                }
                if (slot < 0) continue;

                int space = HTTP_BUF_SZ - clients[slot].buf_len;
                if (space <= 0)
                {
                    log_warning("client buffer full, closing");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients[slot].active = 0;
                    clients[slot].fd = -1;
                    continue;
                }

                int n = recv(fd,
                             clients[slot].buf + clients[slot].buf_len,
                             space - 1, 0);
                if (n <= 0)
                {
                    if (n < 0)
                    {
                        char msg[80];
                        snprintf(msg, sizeof(msg),
                                 "recv error from %s:%d fd=%d",
                                 clients[slot].ip, clients[slot].port, fd);
                        log_warning(msg);
                    }
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients[slot].active = 0;
                    clients[slot].fd = -1;
                    continue;
                }

                clients[slot].buf_len += n;
                clients[slot].buf[clients[slot].buf_len] = '\0';

                /* 判断请求完整性 */
                char *header_end = strstr(clients[slot].buf, "\r\n\r\n");
                if (header_end == NULL)
                    continue;  /* 请求头不全, 等更多数据 */

                /* 有 Content-Length 则检查请求体是否收齐 */
                const char *cl_hdr = strstr(clients[slot].buf,
                                            "Content-Length:");
                if (cl_hdr)
                {
                    int body_len = atoi(cl_hdr + 15);
                    int header_total =
                        header_end - clients[slot].buf + 4;
                    int received = clients[slot].buf_len - header_total;
                    if (received < body_len)
                        continue;  /* 请求体不全, 继续等 */
                }

                /* 请求完整 */
                request_count++;
                process_http_request(slot, epfd, users, doc_root,
                                     routes, route_count);
            }
        }
    }

    log_info("epoll_server: done");
    close(epfd);
    close(listen_fd);
    return 0;
}
