/*
 * W2D5 训练：TCP Client — 连接 EpollServer 收发消息
 *
 * 用法: ./client <server_ip> <port>
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
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int sock_fd;
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "invalid server IP: %s\n", server_ip);
        close(sock_fd);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to %s:%d. Type messages (quit to exit):\n",
           server_ip, port);

    while (1)
    {
        printf("> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';

        int len = strlen(buf);
        buf[len] = '\n';
        send(sock_fd, buf, len + 1, 0);

        if (strcmp(buf, "quit") == 0)
        {
            printf("Client exiting.\n");
            break;
        }
    }

    close(sock_fd);
    return 0;
}
