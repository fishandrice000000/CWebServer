#include "request_handler.h"
#include "user_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_LINE 256

/* W3D2: 前向声明 */
static int send_all(int fd, const char *data, size_t len);
static int normalize_path(const char *raw_path, char *out, size_t out_sz);
static int validate_path(const char *doc_root, const char *req_path,
                         char *real_path, size_t rp_sz);
static int serve_static_file(int conn_fd, const char *real_path,
                             int *status_code);
static int handle_search(int conn_fd, const http_request_t *req,
                         int *status_code);
static int basic_auth_check(const http_request_t *req);
static int base64_decode(const char *src, char *dst, int dst_sz);

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
                        UserNode *users, int *status_code,
                        const char *doc_root,
                        const route_config_t *routes, int route_count)
{
    int status = 200;

    /* ---- V1.4: 配置路由优先 ---- */
    int path_in_routes = 0;
    for (int i = 0; i < route_count; i++)
    {
        if (strcmp(req->method, routes[i].method) == 0 &&
            strcmp(req->path, routes[i].path) == 0)
        {
            /* 命中配置路由 — 先检查认证 */
            if (strcmp(routes[i].auth, "basic") == 0)
            {
                int auth_result = basic_auth_check(req);
                if (auth_result != 0)
                {
                    char *page = "401 Unauthorized\n";
                    int plen = (int)strlen(page);
                    dprintf(conn_fd,
                        "HTTP/1.1 401 Unauthorized\r\n"
                        "WWW-Authenticate: Basic realm=\"mini_webserver\"\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n%s", plen, page);
                    if (status_code) *status_code = 401;
                    return plen;
                }
            }

            if (strcmp(routes[i].handler_name, "users_get") == 0)
            {
                char *page = "<html><body><h1>Users</h1>"
                    "<p>GET /users — list users (placeholder)</p>"
                    "</body></html>";
                int plen = (int)strlen(page);
                int total = dprintf(conn_fd,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "\r\n%s", plen, page);
                if (status_code) *status_code = 200;
                return total;
            }
            if (strcmp(routes[i].handler_name, "users_create") == 0)
            {
                char *page = "<html><body><h1>Users</h1>"
                    "<p>POST /users — create user (placeholder)</p>"
                    "</body></html>";
                int plen = (int)strlen(page);
                int total = dprintf(conn_fd,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "\r\n%s", plen, page);
                if (status_code) *status_code = 200;
                return total;
            }
            if (strcmp(routes[i].handler_name, "secured_get") == 0)
            {
                char *page = "<html><body><h1>Secured Page</h1>"
                    "<p>Welcome to the protected area.</p>"
                    "</body></html>";
                int plen = (int)strlen(page);
                int total = dprintf(conn_fd,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "Cache-Control: no-store\r\n"
                    "\r\n%s", plen, page);
                if (status_code) *status_code = 200;
                return total;
            }
        }
        if (strcmp(req->path, routes[i].path) == 0)
            path_in_routes = 1;
    }
    /* path 在路由表中存在但 method 不匹配 → 405 */
    if (path_in_routes)
    {
        status = 405;
        goto send_error;
    }

    /* ---- GET 请求 ---- */
    if (strcmp(req->method, "GET") == 0)
    {
        /* 保留旧版路由 */
        if (strcmp(req->path, "/hello") == 0)
        {
            const char *h = "Hello, Web!\n";
            int hlen = (int)strlen(h);
            int total = dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n%s", hlen, h);
            if (status_code) *status_code = 200;
            return total;
        }
        if (strncmp(req->path, "/users/", 7) == 0)
        {
            char ub[512];
            int ulen = 0;
            const char *name = req->path + 7;
            UserNode *p = users->next;
            while (p != NULL)
            {
                if (strcmp(p->data.username, name) == 0)
                {
                    ulen = snprintf(ub, sizeof(ub),
                        "FOUND %s %s %s\n",
                        p->data.username, p->data.password, p->data.phone);
                    break;
                }
                p = p->next;
            }
            if (ulen == 0)
                ulen = snprintf(ub, sizeof(ub), "NOT_FOUND %s\n", name);
            int total = dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n%s", ulen, ub);
            if (status_code) *status_code = 200;
            return total;
        }

        /* /search?class=... */
        {
            const char *qm = strchr(req->path, '?');
            size_t plen = qm ? (size_t)(qm - req->path) : strlen(req->path);
            if (plen == 7 && strncmp(req->path, "/search", 7) == 0)
        {
            int total = handle_search(conn_fd, req, &status);
            if (total >= 0)
            {
                if (status_code) *status_code = status;
                return total;
            }
            /* handle_search 失败: 用 send_error 发送错误响应 */
            goto send_error;
        }
        }

        /* 回退到静态文件: 规范化路径: 去 ?参数, / → /index.html */
        char norm[512];
        if (normalize_path(req->path, norm, sizeof(norm)) != 0)
        {
            status = 400;
            goto send_error;
        }

        /* 安全检查 + realpath */
        char real_path[512];
        int sec = validate_path(doc_root, norm, real_path, sizeof(real_path));
        if (sec != 0)
        {
            status = sec;  /* 403, 404 等 */
            goto send_error;
        }

        /* 服务静态文件 */
        int total = serve_static_file(conn_fd, real_path, &status);
        if (total >= 0)
        {
            if (status_code) *status_code = status;
            return total;
        }
        /* 失败 → 发送错误响应 */
        goto send_error;
    }

    /* ---- POST /search ---- */
    if (strncmp(req->path, "/search", 7) == 0 &&
        strcmp(req->method, "POST") == 0)
    {
        int total = handle_search(conn_fd, req, &status);
        if (total >= 0)
        {
            if (status_code) *status_code = status;
            return total;
        }
        goto send_error;
    }

    /* ---- POST /echo ---- */
    if (strcmp(req->path, "/echo") == 0 && strcmp(req->method, "POST") == 0)
    {
        /* 回显请求体 */
        const char *body = req->body;
        int body_len = req->body_len;
        if (body_len > 0)
        {
            int total = dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n", body_len);
            if (total < 0) { status = 500; goto send_error; }
            if (send_all(conn_fd, body, (size_t)body_len) != 0)
            { status = 500; goto send_error; }
            if (status_code) *status_code = 200;
            return total + body_len;
        }
        else
        {
            int total = dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n");
            if (status_code) *status_code = 200;
            return total;
        }
    }

    /* ---- 非 GET/POST → 405 ---- */
    if (strcmp(req->method, "GET") != 0 &&
        strcmp(req->method, "POST") != 0)
    {
        status = 405;
        goto send_error;
    }

    /* ---- 其他 POST 路径 → 404 ---- */
    status = 404;

send_error:
    {
        const char *status_text;
        switch (status) {
        case 400: status_text = "Bad Request"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        default:  status_text = "Internal Server Error"; status = 500; break;
        }

        char err_body[256];
        int err_len = snprintf(err_body, sizeof(err_body),
            "%d %s: %s\n", status, status_text, req->path);
        int total = dprintf(conn_fd,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            status, status_text, err_len);
        if (total < 0) { if (status_code) *status_code = status; return -1; }
        if (send_all(conn_fd, err_body, (size_t)err_len) != 0)
        { if (status_code) *status_code = status; return -1; }
        if (status_code) *status_code = status;
        return total + err_len;
    }
}

/* ================================================================
 * W3D2: 静态资源文件服务
 * ================================================================ */

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/* ---- MIME 类型映射 ---- */
static const char *get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (ext == NULL) return "application/octet-stream";

    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcasecmp(ext, ".css") == 0)
        return "text/css; charset=utf-8";
    if (strcasecmp(ext, ".js") == 0)
        return "text/javascript; charset=utf-8";
    if (strcasecmp(ext, ".json") == 0)
        return "application/json; charset=utf-8";
    if (strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".jpg") == 0)
        return "image/jpeg";
    if (strcasecmp(ext, ".png") == 0)
        return "image/png";
    if (strcasecmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcasecmp(ext, ".ico") == 0)
        return "image/vnd.microsoft.icon";
    if (strcasecmp(ext, ".svg") == 0)
        return "image/svg+xml";
    if (strcasecmp(ext, ".txt") == 0)
        return "text/plain; charset=utf-8";

    return "application/octet-stream";
}

/* ---- 可靠发送: 处理部分发送和 EINTR ---- */
static int send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n > 0)
            sent += (size_t)n;
        else if (n < 0 && errno == EINTR)
            continue;
        else
            return -1;
    }
    return 0;
}

/* ---- 规范化路径: 去查询参数, 映射 / → /index.html ---- */
static int normalize_path(const char *raw_path, char *out, size_t out_sz)
{
    /* 去除 ? 查询参数 */
    const char *q = strchr(raw_path, '?');
    size_t len = q ? (size_t)(q - raw_path) : strlen(raw_path);

    if (len >= out_sz) len = out_sz - 1;
    memcpy(out, raw_path, len);
    out[len] = '\0';

    /* / 映射为 /index.html */
    if (strcmp(out, "/") == 0)
    {
        if (out_sz < 12) return -1;
        strcpy(out, "/index.html");
    }

    return 0;
}

/* ---- 路径安全检查: 拒绝 .., 验证 realpath 在 doc_root 内 ---- */
static int validate_path(const char *doc_root, const char *req_path,
                         char *real_path, size_t rp_sz)
{
    /* 拒绝包含 .. 的路径 */
    if (strstr(req_path, ".."))
        return 403;

    /* 构造完整路径: doc_root + req_path */
    char full[512];
    int n = snprintf(full, sizeof(full), "%s%s", doc_root, req_path);
    if (n < 0 || n >= (int)sizeof(full)) return 400;

    /* realpath 解析符号链接并得到绝对路径 */
    if (realpath(full, real_path) == NULL)
    {
        /* 文件不存在: 404 */
        if (errno == ENOENT || errno == ENOTDIR)
            return 404;
        if (errno == EACCES)
            return 403;
        return 500;
    }

    /* 获取 doc_root 的绝对路径 */
    char root_real[512];
    if (realpath(doc_root, root_real) == NULL)
        return 500;

    size_t root_len = strlen(root_real);

    /* real_path 必须以 root_real 开头 */
    if (strncmp(real_path, root_real, root_len) != 0)
        return 403;
    /* 并且紧接 \0 或 / (防止 /www-evil 匹配 /www) */
    if (real_path[root_len] != '\0' && real_path[root_len] != '/')
        return 403;

    return 0;
}

/* ---- 发送静态文件响应 ---- */
static int serve_static_file(int conn_fd, const char *real_path,
                             int *status_code)
{
    /* stat() 获取文件元数据 */
    struct stat st;
    if (stat(real_path, &st) != 0)
    {
        if (errno == ENOENT || errno == ENOTDIR)
            *status_code = 404;
        else if (errno == EACCES)
            *status_code = 403;
        else
            *status_code = 500;
        return -1;
    }

    /* 只允许普通文件 */
    if (!S_ISREG(st.st_mode))
    {
        *status_code = 403;
        return -1;
    }

    /* 打开文件 */
    int file_fd = open(real_path, O_RDONLY);
    if (file_fd < 0)
    {
        if (errno == EACCES) *status_code = 403;
        else if (errno == ENOENT) *status_code = 404;
        else *status_code = 500;
        return -1;
    }

    /* MIME */
    const char *mime = get_mime_type(real_path);

    /* 发送响应头 */
    off_t file_size = st.st_size;
    int header_len = dprintf(conn_fd,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n", mime, (long)file_size);
    if (header_len < 0) { close(file_fd); *status_code = 500; return -1; }

    /* 分块发送文件 body */
    char buf[8192];
    ssize_t nr;
    int total = header_len;
    while ((nr = read(file_fd, buf, sizeof(buf))) > 0)
    {
        if (send_all(conn_fd, buf, (size_t)nr) != 0)
        {
            close(file_fd);
            *status_code = 500;
            return -1;
        }
        total += (int)nr;
    }

    close(file_fd);
    *status_code = 200;
    return total;
}

/* ================================================================
 * W3D3: GET/POST 动态查询
 * ================================================================ */

/* ---- URL 解码: %HH → 字符, + → 空格 ---- */
static int url_decode(const char *src, char *dst, int dst_sz)
{
    int j = 0;
    for (int i = 0; src[i] && j < dst_sz - 1; i++)
    {
        if (src[i] == '+')
        {
            dst[j++] = ' ';
        }
        else if (src[i] == '%' && src[i+1] && src[i+2])
        {
            char hex[3] = { src[i+1], src[i+2], '\0' };
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 2;
        }
        else
        {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
    return j;
}

/* ---- 查询参数解析: 从 query string 提取指定 key 的值 ---- */
static const char *get_query_param(const char *query, const char *key)
{
    if (query == NULL || query[0] == '\0') return NULL;
    static char value[256];
    int key_len = strlen(key);

    const char *p = query;
    while (*p)
    {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=')
        {
            p += key_len + 1;
            const char *end = strchr(p, '&');
            int vlen = end ? (int)(end - p) : (int)strlen(p);
            if (vlen >= (int)sizeof(value)) vlen = (int)sizeof(value) - 1;
            url_decode(p, value, vlen + 1);
            return value;
        }
        p = strchr(p, '&');
        if (p == NULL) break;
        p++;
    }
    return NULL;
}

/* ---- HTML 转义: 防止注入 ---- */
static void html_escape(const char *src, char *dst, int dst_sz)
{
    int j = 0;
    for (int i = 0; src[i] && j < dst_sz - 8; i++)
    {
        switch (src[i]) {
        case '&':  memcpy(dst + j, "&amp;", 5);  j += 5; break;
        case '<':  memcpy(dst + j, "&lt;", 4);   j += 4; break;
        case '>':  memcpy(dst + j, "&gt;", 4);   j += 4; break;
        case '"':  memcpy(dst + j, "&quot;", 6); j += 6; break;
        case '\'': memcpy(dst + j, "&#39;", 5);  j += 5; break;
        default:   dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

/* ---- 生成查询表单 HTML ---- */
static int build_search_form(char *buf, int buf_sz)
{
    return snprintf(buf, buf_sz,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><meta charset=\"UTF-8\">"
        "<title>Search</title></head>\n"
        "<body>\n"
        "<h1>Student Search</h1>\n"
        "<form action=\"/search\" method=\"GET\""
        " accept-charset=\"UTF-8\">\n"
        "<label>Class: <input name=\"class\""
        " placeholder=\"e.g. 2011\"></label><br>\n"
        "<label>Keyword: <input name=\"keyword\""
        " placeholder=\"name or id\"></label><br>\n"
        "<button type=\"submit\">Search</button>\n"
        "</form>\n"
        "<p>Also try POST /search with"
        " application/x-www-form-urlencoded</p>\n"
        "</body>\n"
        "</html>\n");
}

/* ---- 搜索数据文件并生成结果 HTML ---- */
static int build_search_result(const char *class_val, const char *keyword,
                                char *buf, int buf_sz)
{
    /* class 校验: 4位数字 */
    if (strlen(class_val) != 4) return -1;
    for (int i = 0; i < 4; i++)
        if (class_val[i] < '0' || class_val[i] > '9') return -1;

    /* keyword 校验: 1~64字节 */
    int kw_len = strlen(keyword);
    if (kw_len < 1 || kw_len > 64) return -1;

    /* 构造文件路径: data/<class>.txt */
    char fpath[128];
    snprintf(fpath, sizeof(fpath), "data/%s.txt", class_val);

    /* 安全检查: realpath 限定在 data/ 内 */
    char real_fpath[512];
    if (realpath(fpath, real_fpath) == NULL)
        return -2;  /* 文件不存在 */

    char data_root[512];
    if (realpath("data", data_root) == NULL) return -2;
    size_t dr_len = strlen(data_root);
    if (strncmp(real_fpath, data_root, dr_len) != 0 ||
        (real_fpath[dr_len] != '/' && real_fpath[dr_len] != '\0'))
        return -2;

    /* 打开文件搜索 */
    FILE *fp = fopen(real_fpath, "r");
    if (fp == NULL) return -2;

    /* 先生成 HTML 头部 */
    int pos = snprintf(buf, buf_sz,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><meta charset=\"UTF-8\">"
        "<title>Search Result</title></head>\n"
        "<body>\n"
        "<h1>Search: class=%s, keyword=%s</h1>\n"
        "<table border=\"1\"><tr>"
        "<th>ID</th><th>Name</th><th>Gender</th></tr>\n",
        class_val, keyword);

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp))
    {
        /* 去掉换行 */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (line[0] == '\0') continue;

        /* 在整行中搜索 keyword */
        if (strstr(line, keyword) == NULL) continue;

        /* 按 \t 拆分: id, name, gender */
        char *id     = strtok(line, "\t");
        char *name   = strtok(NULL, "\t");
        char *gender = strtok(NULL, "\t");
        if (id == NULL || name == NULL || gender == NULL) continue;

        char eid[64], ename[128], egender[16];
        html_escape(id,     eid,     sizeof(eid));
        html_escape(name,   ename,   sizeof(ename));
        html_escape(gender, egender, sizeof(egender));

        pos += snprintf(buf + pos, buf_sz - pos,
            "<tr><td>%s</td><td>%s</td><td>%s</td></tr>\n",
            eid, ename, egender);
        found++;
    }
    fclose(fp);

    if (!found)
        pos += snprintf(buf + pos, buf_sz - pos,
            "<tr><td colspan=\"3\">No results found</td></tr>\n");

    pos += snprintf(buf + pos, buf_sz - pos,
        "</table>\n"
        "<p><a href=\"/search\">Back to search</a></p>\n"
        "</body>\n</html>\n");

    return pos;  /* 返回总字节数 */
}

/* ---- 处理 /search 路由 (GET 表单 / GET 查询 / POST 查询) ---- */
static int handle_search(int conn_fd, const http_request_t *req,
                         int *status_code)
{
    const char *query = NULL;
    char decoded_body[4096];

    /* 确定参数来源 */
    if (strcmp(req->method, "GET") == 0)
    {
        /* GET: 从 URL 中提取 ? 后面的查询字符串 */
        const char *qmark = strchr(req->path, '?');
        if (qmark)
        {
            /* 路径中有 ? → 查询模式 */
            query = qmark + 1;
        }
        else
        {
            /* 无 ? → 返回查询表单 */
            char form[4096];
            int form_len = build_search_form(form, sizeof(form));
            int total = dprintf(conn_fd,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n", form_len);
            if (total < 0) { *status_code = 500; return -1; }
            if (send_all(conn_fd, form, (size_t)form_len) != 0)
            { *status_code = 500; return -1; }
            *status_code = 200;
            return total + form_len;
        }
    }
    else if (strcmp(req->method, "POST") == 0)
    {
        /* POST: 从请求体解析 */
        const char *ct = http_get_header(req, "Content-Type");
        if (ct == NULL || strstr(ct, "application/x-www-form-urlencoded") == NULL)
        {
            char eb[64];
            int elen = snprintf(eb, sizeof(eb),
                "415 Unsupported Media Type\n");
            dprintf(conn_fd,
                "HTTP/1.1 415 Unsupported Media Type\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n\r\n", elen);
            send_all(conn_fd, eb, (size_t)elen);
            *status_code = 415;
            return -1;
        }
        if (req->body_len > 4096)
        {
            char eb[64];
            int elen = snprintf(eb, sizeof(eb),
                "413 Content Too Large\n");
            dprintf(conn_fd,
                "HTTP/1.1 413 Content Too Large\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n\r\n", elen);
            send_all(conn_fd, eb, (size_t)elen);
            *status_code = 413;
            return -1;
        }
        memcpy(decoded_body, req->body, req->body_len);
        decoded_body[req->body_len] = '\0';
        query = decoded_body;
    }
    else
    {
        *status_code = 405;
        return -1;
    }

    /* 解析参数 */
    /* get_query_param 使用 static 缓冲区, 每次调用后立即复制 */
    char class_buf[16], kw_buf[128];
    const char *class_val = NULL;
    const char *keyword   = NULL;
    {
        const char *v = get_query_param(query, "class");
        if (v) { strncpy(class_buf, v, sizeof(class_buf)-1);
                 class_buf[sizeof(class_buf)-1]='\0'; class_val=class_buf; }
    }
    {
        const char *v = get_query_param(query, "keyword");
        if (v) { strncpy(kw_buf, v, sizeof(kw_buf)-1);
                 kw_buf[sizeof(kw_buf)-1]='\0'; keyword=kw_buf; }
    }

    if (class_val == NULL || keyword == NULL)
    {
        *status_code = 400;
        return -1;
    }

    /* 搜索 */
    char result[16384];
    int result_len = build_search_result(class_val, keyword,
                                          result, sizeof(result));
    if (result_len < 0)
    {
        if (result_len == -2) *status_code = 404;
        else *status_code = 400;
        return -1;
    }

    /* 发送 HTML 结果 */
    int total = dprintf(conn_fd,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n", result_len);
    if (total < 0) { *status_code = 500; return -1; }
    if (send_all(conn_fd, result, (size_t)result_len) != 0)
    { *status_code = 500; return -1; }
    *status_code = 200;
    return total + result_len;
}

/* ================================================================
 * W3D5: HTTP Basic 认证
 * ================================================================ */

/* ---- Base64 解码 ---- */
static int base64_decode(const char *src, char *dst, int dst_sz)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char buf[4];
    int buf_len = 0;
    while (src[i] && src[i] != '=' && j < dst_sz - 1)
    {
        const char *p = strchr(table, src[i]);
        if (p == NULL) { i++; continue; }
        buf[buf_len++] = (unsigned char)(p - table);
        if (buf_len == 4)
        {
            dst[j++] = (char)((buf[0] << 2) | (buf[1] >> 4));
            if (j < dst_sz - 1 && buf[2] < 64)
                dst[j++] = (char)((buf[1] << 4) | (buf[2] >> 2));
            if (j < dst_sz - 1 && buf[3] < 64)
                dst[j++] = (char)((buf[2] << 6) | buf[3]);
            buf_len = 0;
        }
        i++;
    }
    /* 处理残留的不完整组 (因 = 或字符串结束而截断) */
    if (buf_len >= 2)
    {
        dst[j++] = (char)((buf[0] << 2) | (buf[1] >> 4));
        if (buf_len >= 3 && j < dst_sz - 1)
            dst[j++] = (char)((buf[1] << 4) | (buf[2] >> 2));
    }
    dst[j] = '\0';
    return j;
}

/* ---- Basic 认证校验 ---- */
static int basic_auth_check(const http_request_t *req)
{
    const char *auth_hdr = http_get_header(req, "Authorization");
    if (auth_hdr == NULL) return 401;

    /* 检查 scheme 是否为 Basic */
    if (strncasecmp(auth_hdr, "Basic ", 6) != 0) return 401;

    /* Base64 解码 */
    char decoded[256];
    base64_decode(auth_hdr + 6, decoded, sizeof(decoded));

    /* 按第一个冒号拆分 username:password */
    char *colon = strchr(decoded, ':');
    if (colon == NULL) return 401;
    *colon = '\0';
    const char *username = decoded;
    const char *password = colon + 1;

    /* 校验凭据 */
    if (strcmp(username, "student") == 0 &&
        strcmp(password, "lab123") == 0)
        return 0;  /* 通过 */

    return 401;  /* 凭据错误 */
}
