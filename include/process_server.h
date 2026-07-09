#ifndef PROCESS_SERVER_H
#define PROCESS_SERVER_H

#include "user_store.h"

/*
 * 扫描 req_dir 下的 .req 文件, 为每个文件 fork 子进程处理,
 * 结果写入 out_dir 下同名的 .out 文件.
 * users: 已加载的用户链表 (父进程传递, 避免子进程重复 I/O).
 * 父进程通过 waitpid 等待所有子进程结束.
 * 成功返回 0, 失败返回 -1.
 */
int process_requests(const char *req_dir, const char *out_dir, UserNode *users);

#endif
