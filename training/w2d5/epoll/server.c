/*
 * W2D5 训练：EpollServer — 基于 epoll 的多客户端文本通信
 *
 * 用法: ./EpollServer <ip> <port>
 *
 * 仅使用 epoll API (epoll_create / epoll_ctl / epoll_wait).
 * LT 模式 (默认), EPOLLIN 监听可读事件.
 * 维护每个客户端的接收缓冲区, 按 \n 分帧.
 * quit\n 或 recv()==0 时 epoll_ctl(DEL) + close.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 256
#define MAX_CLIENTS 256
#define BUF_SIZE    4096

typedef struct {
    int  fd;
    int  active;
    char ip[INET_ADDRSTRLEN];
    int  port;
    char buf[BUF_SIZE];
    int  buf_len;
} client_t;

static client_t clients[MAX_CLIENTS];
static int      client_count = 0;

/* ---- 分帧处理: 与 W2D4 相同 ---- */
static void process_messages(int idx, int epfd)
{
    client_t *c = &clients[idx];
    char *data = c->buf;
    int   len  = c->buf_len;

    while (1)
    {
        char *nl = memchr(data, '\n', len);
        if (nl == NULL) break;

        int msg_len = nl - data;
        char msg[1024];
        if (msg_len >= (int)sizeof(msg))
            msg_len = (int)sizeof(msg) - 1;
        memcpy(msg, data, msg_len);
        msg[msg_len] = '\0';

        printf("[%s:%d] %s\n", c->ip, c->port, msg);

        if (strcmp(msg, "quit") == 0)
        {
            printf("[server] %s:%d sent quit, closing.\n",
                   c->ip, c->port);
            epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
            close(c->fd);
            c->active = 0;
            c->fd = -1;
            client_count--;
            return;
        }

        data += msg_len + 1;
        len  -= msg_len + 1;
    }

    if (data != c->buf && len > 0)
        memmove(c->buf, data, len);
    c->buf_len = len;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    int listen_fd, epfd;
    struct sockaddr_in server_addr;
    struct epoll_event ev, events[MAX_EVENTS];

    memset(clients, 0, sizeof(clients));
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    /* 1. socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "invalid IP: %s\n", ip);
        close(listen_fd);
        return 1;
    }

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

    /* 4. epoll_create */
    epfd = epoll_create(1);
    if (epfd < 0) { perror("epoll_create"); close(listen_fd); return 1; }

    /* 5. 注册 listen_fd 到 epoll */
    ev.events  = EPOLLIN;                     /* LT 模式, 监听可读 */
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        perror("epoll_ctl: listen_fd");
        close(epfd);
        close(listen_fd);
        return 1;
    }

    printf("EpollServer listening on %s:%d ...\n", ip, port);

    /* 6. 主循环 */
    while (1)
    {
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

            /* ---- 新连接 ---- */
            if (fd == listen_fd)
            {
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

                /* 找空闲 slot */
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

                    /* 注册到 epoll */
                    ev.events  = EPOLLIN;
                    ev.data.fd = conn_fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev) < 0)
                    {
                        perror("epoll_ctl: conn_fd");
                        close(conn_fd);
                        clients[slot].active = 0;
                        clients[slot].fd = -1;
                        continue;
                    }

                    client_count++;
                    printf("[server] %s:%d connected (total: %d)\n",
                           clients[slot].ip, clients[slot].port,
                           client_count);
                }
            }
            /* ---- 客户端数据 ---- */
            else
            {
                /* 找到对应 slot */
                int slot = -1;
                for (int j = 0; j < MAX_CLIENTS; j++)
                {
                    if (clients[j].active && clients[j].fd == fd)
                    { slot = j; break; }
                }
                if (slot < 0) continue;

                int space = BUF_SIZE - clients[slot].buf_len;
                if (space <= 0)
                {
                    fprintf(stderr, "buffer full for %s:%d, closing.\n",
                            clients[slot].ip, clients[slot].port);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients[slot].active = 0;
                    clients[slot].fd = -1;
                    client_count--;
                    continue;
                }

                int n = recv(fd, clients[slot].buf + clients[slot].buf_len,
                             space - 1, 0);
                if (n <= 0)
                {
                    if (n == 0)
                        printf("[server] %s:%d disconnected "
                               "(total: %d)\n",
                               clients[slot].ip, clients[slot].port,
                               client_count - 1);
                    else
                        fprintf(stderr, "recv error from %s:%d\n",
                                clients[slot].ip, clients[slot].port);

                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients[slot].active = 0;
                    clients[slot].fd = -1;
                    client_count--;
                }
                else
                {
                    clients[slot].buf_len += n;
                    clients[slot].buf[clients[slot].buf_len] = '\0';
                    process_messages(slot, epfd);
                }
            }
        }
    }

    close(epfd);
    close(listen_fd);
    return 0;
}
