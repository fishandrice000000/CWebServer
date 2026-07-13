/*
 * W2D1 训练：TCP 文本聊天 — 服务器端
 *
 * 用法: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

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
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    /* 2. bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

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
    printf("Server listening on port %d, waiting for connection...\n", port);

    /* 4. accept */
    conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (conn_fd < 0)
    {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("Client connected: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    /* 5. 聊天循环 */
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        int n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        /* 去换行 */
        char *nl = strchr(buf, '\n');
        if (nl)
            *nl = '\0';

        printf("[Client] %s\n", buf);

        /* 检查退出 */
        if (strcmp(buf, "exit") == 0)
        {
            printf("Client requested exit. Server shutting down.\n");
            break;
        }

        /* 回复 */
        printf("Server reply: ");
        fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        nl = strchr(buf, '\n');
        if (nl)
            *nl = '\0';

        send(conn_fd, buf, strlen(buf), 0);

        if (strcmp(buf, "exit") == 0)
        {
            printf("Server exiting.\n");
            break;
        }
    }

    close(conn_fd);
    close(listen_fd);
    return 0;
}
