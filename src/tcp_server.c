#include "tcp_server.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 1024

int tcp_server_run(const char *host, int port, UserNode *users)
{
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /* 1. socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return -1; }

    /* 允许端口快速重用 (避免 TIME_WAIT 导致 bind 失败) */
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (strcmp(host, "0.0.0.0") == 0 || strlen(host) == 0)
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "tcp_server: invalid host %s\n", host);
        close(listen_fd);
        return -1;
    }

    if (bind(listen_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    /* 3. listen */
    if (listen(listen_fd, 5) < 0) { perror("listen"); close(listen_fd); return -1; }

    char msg[128];
    snprintf(msg, sizeof(msg), "tcp_server: listening on %s:%d", host, port);
    log_info(msg);
    printf("Server listening on %s:%d ...\n", host, port);

    /* 4. accept — 单连接 */
    conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (conn_fd < 0) { perror("accept"); close(listen_fd); return -1; }

    snprintf(msg, sizeof(msg), "tcp_server: connected from %s:%d",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    log_info(msg);

    /* 5. 处理 HTTP 请求 */
    handle_http_request(conn_fd, users);

    log_info("tcp_server: done");
    close(conn_fd);
    close(listen_fd);
    return 0;
}
