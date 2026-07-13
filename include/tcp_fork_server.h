#ifndef TCP_FORK_SERVER_H
#define TCP_FORK_SERVER_H

#include "user_store.h"

/*
 * 启动多进程 TCP 服务器, 监听 config 中配置的 host:port.
 * 每 accept 一个客户端就 fork 子进程处理, 父进程继续 accept.
 * 注册 SIGCHLD 回收子进程, 处理 EINTR, 忽略 SIGPIPE.
 * host:        监听地址
 * port:        监听端口
 * users:       已加载的用户链表
 * max_clients: 处理多少个客户端后退出 (0 表示不限)
 * 成功返回 0, 失败返回 -1.
 */
int tcp_fork_server_run(const char *host, int port, UserNode *users,
                        int max_clients);

#endif
