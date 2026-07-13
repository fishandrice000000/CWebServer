#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "user_store.h"

/*
 * 处理一个请求文件, 将结果写入输出文件.
 * req_path: 请求文件路径 (如 requests/hello.req)
 * out_path: 输出文件路径 (如 outputs/hello.out)
 * users:    已加载的用户链表 (避免每个子进程重复加载 CSV)
 * 成功返回 0, 失败返回 -1.
 */
int handle_request(const char *req_path, const char *out_path, UserNode *users);

/*
 * 从 TCP 连接读取 HTTP 请求, 解析第一行 ("GET /path HTTP/1.1"),
 * 路由到对应处理函数, 将响应写回 conn_fd.
 * 支持: /hello → 200, /users/<name> → FOUND/NOT_FOUND, 其他 → 404.
 * 成功返回 0.
 */
int handle_http_request(int conn_fd, UserNode *users);

#endif
