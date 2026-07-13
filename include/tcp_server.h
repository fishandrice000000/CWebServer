#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "user_store.h"

/*
 * 启动 TCP 服务器, 监听 config 中配置的 host:port.
 * 接受一个客户端连接, 处理 HTTP 请求后退出.
 * users: 已加载的用户链表.
 * 成功返回 0, 失败返回 -1.
 */
int tcp_server_run(const char *host, int port, UserNode *users);

#endif
