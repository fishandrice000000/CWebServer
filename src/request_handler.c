#include "request_handler.h"
#include "user_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
    char body[256];

    /* 只读 HTTP 请求的第一行 */
    if (read_line(conn_fd, line, sizeof(line)) < 0) {
        dprintf(conn_fd,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
        return -1;
    }

    char method[8], path[128];
    if (sscanf(line, "%7s %127s", method, path) != 2) {
        dprintf(conn_fd,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
        return -1;
    }

    if (strcmp(path, "/hello") == 0) {
        const char *hello_body = "Hello, Web!\n";
        int hello_len = (int)strlen(hello_body);
        dprintf(conn_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", hello_len, hello_body);

    } else if (strncmp(path, "/users/", 7) == 0) {
        const char *name = path + 7;
        UserNode *p = users->next;
        int found = 0;
        while (p != NULL) {
            if (strcmp(p->data.username, name) == 0) {
                int body_len = snprintf(body, sizeof(body),
                    "FOUND %s %s %s\n",
                    p->data.username, p->data.password, p->data.phone);
                dprintf(conn_fd,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s", body_len, body);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (!found) {
            int body_len = snprintf(body, sizeof(body),
                "NOT_FOUND %s\n", name);
            dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "\r\n"
                "%s", body_len, body);
        }

    } else {
        dprintf(conn_fd,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    }

    return 0;
}

/* ================================================================
 * W3D1: 完整 HTTP 请求解析与处理
 * ================================================================ */

const char *http_get_header(const http_request_t *req, const char *name)
{
    static char value[512];
    const char *p = req->headers;
    int name_len = strlen(name);

    while (*p)
    {
        if (strncasecmp(p, name, name_len) == 0 && p[name_len] == ':')
        {
            p += name_len + 1;
            while (*p == ' ' || *p == '\t') p++;

            /* 拷贝值到静态缓冲区, 在 \r\n 处截断 */
            const char *end = strstr(p, "\r\n");
            int vlen = end ? (int)(end - p) : (int)strlen(p);
            if (vlen >= (int)sizeof(value)) vlen = (int)sizeof(value) - 1;
            memcpy(value, p, vlen);
            value[vlen] = '\0';
            return value;
        }
        const char *nl = strstr(p, "\r\n");
        if (nl == NULL) break;
        p = nl + 2;
    }
    return NULL;
}

int http_parse_request(const char *data, int len, http_request_t *req)
{
    if (data == NULL || len <= 0) return -1;

    memset(req, 0, sizeof(*req));

    const char *header_end = strstr(data, "\r\n\r\n");
    if (header_end == NULL) return -1;

    const char *line_start = data;
    const char *line_end   = strstr(line_start, "\r\n");
    if (line_end == NULL) return -1;

    char first_line[1024];
    int first_line_len = line_end - line_start;
    if (first_line_len >= (int)sizeof(first_line))
        first_line_len = (int)sizeof(first_line) - 1;
    memcpy(first_line, line_start, first_line_len);
    first_line[first_line_len] = '\0';

    if (sscanf(first_line, "%15s %255s %15s",
               req->method, req->path, req->version) < 2)
        return -1;

    int hdr_start = line_end - data + 2;
    int hdr_len   = header_end - data - hdr_start;
    if (hdr_len > 0)
    {
        if (hdr_len >= HTTP_MAX_HEADERS) hdr_len = HTTP_MAX_HEADERS - 1;
        memcpy(req->headers, data + hdr_start, hdr_len);
        req->headers[hdr_len] = '\0';
    }

    const char *cl = http_get_header(req, "Content-Length");
    int body_len = 0;
    if (cl) body_len = atoi(cl);

    if (body_len > 0)
    {
        const char *body_start = header_end + 4;
        int remaining = len - (body_start - data);
        if (remaining >= body_len)
        {
            if (body_len >= HTTP_MAX_BODY) body_len = HTTP_MAX_BODY - 1;
            memcpy(req->body, body_start, body_len);
            req->body[body_len] = '\0';
            req->body_len = body_len;
        }
    }

    return 0;
}

int http_handle_request(int conn_fd, const http_request_t *req,
                        UserNode *users)
{
    char body[4096];
    int  body_len   = 0;
    int  status     = 200;
    const char *status_text = "OK";
    const char *content_type = "text/html; charset=utf-8";

    if (strcmp(req->path, "/") == 0 && strcmp(req->method, "GET") == 0)
    {
        body_len = snprintf(body, sizeof(body),
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>MiniWeb</title></head>\n"
            "<body>\n"
            "<h1>Hello, HTTP!</h1>\n"
            "</body>\n"
            "</html>\n");
    }
    else if (strcmp(req->path, "/hello") == 0 &&
             strcmp(req->method, "GET") == 0)
    {
        content_type = "text/plain";
        body_len = snprintf(body, sizeof(body), "Hello, Web!\n");
    }
    else if (strncmp(req->path, "/users/", 7) == 0 &&
             strcmp(req->method, "GET") == 0)
    {
        content_type = "text/plain";
        const char *name = req->path + 7;
        UserNode *p = users->next;
        int found = 0;
        while (p != NULL)
        {
            if (strcmp(p->data.username, name) == 0)
            {
                body_len = snprintf(body, sizeof(body),
                    "FOUND %s %s %s\n",
                    p->data.username, p->data.password, p->data.phone);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (!found)
            body_len = snprintf(body, sizeof(body),
                "NOT_FOUND %s\n", name);
    }
    else if (strcmp(req->path, "/echo") == 0 &&
             strcmp(req->method, "POST") == 0)
    {
        content_type = "text/plain";
        if (req->body_len > 0)
        {
            body_len = req->body_len;
            if (body_len >= (int)sizeof(body))
                body_len = (int)sizeof(body) - 1;
            memcpy(body, req->body, body_len);
            body[body_len] = '\0';
        }
    }
    else if (strncmp(req->path, "/missing", 8) == 0)
    {
        status = 404;
        status_text = "Not Found";
        content_type = "text/plain";
        body_len = snprintf(body, sizeof(body),
            "404 Not Found: %s\n", req->path);
    }
    else if (strcmp(req->method, "GET") != 0 &&
             strcmp(req->method, "POST") != 0)
    {
        status = 405;
        status_text = "Method Not Allowed";
        content_type = "text/plain";
        body_len = snprintf(body, sizeof(body),
            "405 Method Not Allowed: %s\n", req->method);
    }
    else
    {
        status = 404;
        status_text = "Not Found";
        content_type = "text/plain";
        body_len = snprintf(body, sizeof(body),
            "404 Not Found: %s\n", req->path);
    }

    int total = dprintf(conn_fd,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, content_type, body_len);
    if (total < 0) return -1;

    if (body_len > 0)
    {
        ssize_t sent = send(conn_fd, body, body_len, 0);
        if (sent < 0) return -1;
        total += (int)sent;
    }

    return total;
}
