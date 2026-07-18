#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "user_store.h"

/* ---- 已有的文件模式请求处理 (V0.2-V0.7) ---- */
int handle_request(const char *req_path, const char *out_path, UserNode *users);

/* ---- 已有的简单 TCP HTTP 处理 (V0.6-V0.8) ---- */
int handle_http_request(int conn_fd, UserNode *users);

/* ---- 解析后的 HTTP 请求结构 (W3D1) ---- */
#define HTTP_MAX_HEADERS 4096
#define HTTP_MAX_BODY    8192

typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char headers[HTTP_MAX_HEADERS];
    char body[HTTP_MAX_BODY];
    int  body_len;
} http_request_t;

/*
 * 从完整 HTTP 请求数据中解析请求行、请求头和请求体.
 * data: 完整的 HTTP 请求数据 (含 \r\n\r\n)
 * len:  数据长度
 * req:  输出, 解析结果
 * 成功返回 0, 解析失败返回 -1.
 */
int http_parse_request(const char *data, int len, http_request_t *req);

/*
 * 处理已解析的 HTTP 请求, 构造响应写入 conn_fd.
 * 返回响应的字节数 (用于访问日志), 失败返回 -1.
 * status_code: 输出参数, 实际返回的 HTTP 状态码.
 * 路由: GET /, GET /hello, GET /users/<name>, POST /echo,
 *       GET /missing → 404, 其他 → 404, 非 GET/POST → 405.
 */
int http_handle_request(int conn_fd, const http_request_t *req,
                        UserNode *users, int *status_code);

/*
 * 从请求头中提取指定字段的值.
 * 如 get_header(req, "Content-Length") → "42".
 * 未找到返回 NULL.
 */
const char *http_get_header(const http_request_t *req, const char *name);

#endif
