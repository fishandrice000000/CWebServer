#include "request_handler.h"
#include "user_store.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_LINE 256

/*
 * 请求格式: "GET /<path>"
 * 支持:
 *   GET /hello        → HTTP 200 + "Hello, Web!"
 *   GET /users/<name> → 查 users.csv, 输出 FOUND/NOT_FOUND
 *   其他              → HTTP 404
 */
int handle_request(const char *req_path, const char *out_path, UserNode *users)
{
    FILE *fp_in = fopen(req_path, "r");
    if (fp_in == NULL) {
        fprintf(stderr, "handle_request: cannot open %s\n", req_path);
        return -1;
    }

    char line[MAX_LINE];
    if (fgets(line, sizeof(line), fp_in) == NULL) {
        fclose(fp_in);
        return -1;
    }
    fclose(fp_in);

    /* 去换行 */
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    FILE *fp_out = fopen(out_path, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "handle_request: cannot write %s\n", out_path);
        return -1;
    }

    /* 解析 "GET /xxx" */
    char method[8], path[128];
    if (sscanf(line, "%7s %127s", method, path) != 2) {
        fprintf(fp_out, "HTTP/1.1 400 Bad Request\n");
        fclose(fp_out);
        return -1;
    }

    if (strcmp(path, "/hello") == 0) {
        fprintf(fp_out,
            "HTTP/1.1 200 OK\n"
            "Content-Type: text/html\n"
            "Content-Length: 12\n"
            "\n"
            "Hello, Web!\n");

    } else if (strncmp(path, "/users/", 7) == 0) {
        const char *name = path + 7;

        /* 使用父进程传入的用户链表, 不重复加载 */
        UserNode *p = users->next;
        int found = 0;
        while (p != NULL) {
            if (strcmp(p->data.username, name) == 0) {
                fprintf(fp_out, "FOUND %s %s %s\n",
                        p->data.username, p->data.password, p->data.phone);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (!found) {
            fprintf(fp_out, "NOT_FOUND %s\n", name);
        }

    } else {
        fprintf(fp_out, "HTTP/1.1 404 Not Found\n");
    }

    fclose(fp_out);
    return 0;
}

/*
 * 从 socket fd 读一行 (直到 \r\n 或 \n), 最多 max-1 字节.
 */
static int read_line(int fd, char *buf, int max)
{
    int i = 0;
    char c;
    while (i < max - 1) {
        int n = recv(fd, &c, 1, 0);
        if (n <= 0) return -1;
        if (c == '\n') break;
        if (c != '\r') buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

int handle_http_request(int conn_fd, UserNode *users)
{
    char line[MAX_LINE];

    /* 只读 HTTP 请求的第一行 */
    if (read_line(conn_fd, line, sizeof(line)) < 0) {
        dprintf(conn_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return -1;
    }

    char method[8], path[128];
    if (sscanf(line, "%7s %127s", method, path) != 2) {
        dprintf(conn_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return -1;
    }

    if (strcmp(path, "/hello") == 0) {
        dprintf(conn_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 12\r\n"
            "\r\n"
            "Hello, Web!\n");

    } else if (strncmp(path, "/users/", 7) == 0) {
        const char *name = path + 7;
        UserNode *p = users->next;
        int found = 0;
        while (p != NULL) {
            if (strcmp(p->data.username, name) == 0) {
                dprintf(conn_fd,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\n"
                    "FOUND %s %s %s\n",
                    p->data.username, p->data.password, p->data.phone);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (!found) {
            dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n"
                "NOT_FOUND %s\n", name);
        }

    } else {
        dprintf(conn_fd,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    }

    return 0;
}
