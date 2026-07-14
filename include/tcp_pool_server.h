#ifndef TCP_POOL_SERVER_H
#define TCP_POOL_SERVER_H

#include "user_store.h"

/*
 * 启动线程池 TCP 服务器, 监听 config 中配置的 host:port.
 * 主线程 accept, client_fd 放入线程池任务队列.
 * num_workers: 线程池大小
 * max_clients: 处理多少个客户端后停止 accept (0 表示不限)
 * 成功返回 0, 失败返回 -1.
 */
int tcp_pool_server_run(const char *host, int port, UserNode *users,
                        int num_workers, int max_clients);

#endif
