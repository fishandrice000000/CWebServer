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

#endif
