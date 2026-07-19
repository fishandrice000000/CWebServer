#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include "user_store.h"

/*
 * 启动 epoll HTTP 服务器, 监听 host:port.
 * 使用 epoll LT 模式, EPOLLIN 监听可读事件.
 * max_requests: 处理多少个请求后停止 (0 表示不限)
 * 成功返回 0, 失败返回 -1.
 */
int epoll_server_run(const char *host, int port, UserNode *users,
                     int max_requests, const char *doc_root);

#endif
