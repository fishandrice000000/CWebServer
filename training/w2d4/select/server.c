/*
 * W2D4 训练：SelectServer — 基于 select 的多客户端文本通信
 *
 * 用法: ./SelectServer <ip> <port>
 *
 * 仅使用 select API, 不使用多线程/多进程/线程池.
 * 维护每个客户端的接收缓冲区, 按 \n 分帧.
 * quit\n 或 recv()==0 时清理客户端.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS  256
#define BUF_SIZE     4096

typedef struct {
    int  fd;
    int  active;
    char ip[INET_ADDRSTRLEN];
    int  port;
    char buf[BUF_SIZE];     /* 接收缓冲区 */
    int  buf_len;           /* 缓冲区中已有的数据长度 */
} client_t;

static client_t clients[MAX_CLIENTS];
static int      client_count = 0;

/* 从客户端缓冲区中提取并处理以 \n 结尾的完整消息 */
static void process_messages(int idx)
{
    client_t *c = &clients[idx];
    char *data = c->buf;
    int   len  = c->buf_len;

    while (1)
    {
        char *nl = memchr(data, '\n', len);
        if (nl == NULL)
            break;                          /* 没有完整消息, 等更多数据 */

        int msg_len = nl - data;
        char msg[1024];
        if (msg_len >= (int)sizeof(msg))
            msg_len = (int)sizeof(msg) - 1;
        memcpy(msg, data, msg_len);
        msg[msg_len] = '\0';

        printf("[%s:%d] %s\n", c->ip, c->port, msg);

        /* 检查 quit */
        if (strcmp(msg, "quit") == 0)
        {
            printf("[server] %s:%d sent quit, closing.\n", c->ip, c->port);
            close(c->fd);
            c->active = 0;
            c->fd = -1;
            client_count--;
            return;
        }

        /* 跳过已处理的消息和 \n */
        data += msg_len + 1;
        len  -= msg_len + 1;
    }

    /* 将未处理完的残留数据移到缓冲区头部 */
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

    /* 确保实时输出 */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    int listen_fd;
    struct sockaddr_in server_addr;
    fd_set read_fds, master_fds;
    int max_fd;

    /* 初始化客户端数组 */
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

    printf("SelectServer listening on %s:%d ...\n", ip, port);

    /* 初始化 fd_set */
    FD_ZERO(&master_fds);
    FD_SET(listen_fd, &master_fds);
    max_fd = listen_fd;

    /* 4. 主循环 */
    while (1)
    {
        read_fds = master_fds;          /* select 会修改, 每次要拷贝 */

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        /* ---- 检查 listen_fd: 新连接 ---- */
        if (FD_ISSET(listen_fd, &read_fds))
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
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (!clients[i].active)
                {
                    slot = i;
                    break;
                }
            }

            if (slot < 0)
            {
                fprintf(stderr, "max clients reached, rejecting.\n");
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

                FD_SET(conn_fd, &master_fds);
                if (conn_fd > max_fd) max_fd = conn_fd;

                client_count++;
                printf("[server] %s:%d connected (total: %d)\n",
                       clients[slot].ip, clients[slot].port,
                       client_count);
            }

            ready--;
            if (ready == 0) continue;
        }

        /* ---- 检查所有 client_fd ---- */
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (!clients[i].active)
                continue;

            int fd = clients[i].fd;

            if (!FD_ISSET(fd, &read_fds))
                continue;

            /* 读取数据 */
            int space = BUF_SIZE - clients[i].buf_len;
            if (space <= 0)
            {
                /* 缓冲区满了, 强制断开 */
                fprintf(stderr, "buffer full for %s:%d, closing.\n",
                        clients[i].ip, clients[i].port);
                FD_CLR(fd, &master_fds);
                close(fd);
                clients[i].active = 0;
                clients[i].fd = -1;
                client_count--;
                continue;
            }

            int n = recv(fd, clients[i].buf + clients[i].buf_len,
                         space - 1, 0);
            if (n <= 0)
            {
                /* 客户端断开或出错 */
                if (n == 0)
                    printf("[server] %s:%d disconnected (total: %d)\n",
                           clients[i].ip, clients[i].port,
                           client_count - 1);
                else
                    fprintf(stderr, "recv error from %s:%d\n",
                            clients[i].ip, clients[i].port);

                FD_CLR(fd, &master_fds);
                close(fd);
                clients[i].active = 0;
                clients[i].fd = -1;
                client_count--;
            }
            else
            {
                clients[i].buf_len += n;
                clients[i].buf[clients[i].buf_len] = '\0';
                process_messages(i);

                /* process_messages 可能已关闭此客户端 */
                if (!clients[i].active)
                    FD_CLR(fd, &master_fds);
            }
        }
    }

    close(listen_fd);
    return 0;
}
