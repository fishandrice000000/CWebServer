#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "user_store.h"

#define MAX_QUEUE 64

/*
 * 任务队列: 循环队列存储 client_fd.
 * mutex 保护队列, not_empty 条件变量用于 worker 等待.
 */
typedef struct {
    int             client_fds[MAX_QUEUE];
    int             head;
    int             tail;
    int             count;
    int             shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} task_queue_t;

/*
 * 线程池: 包含任务队列 + worker 线程数组 + users 指针.
 */
typedef struct {
    int             num_workers;
    pthread_t      *workers;
    task_queue_t    queue;
    UserNode       *users;
} thread_pool_t;

/*
 * 初始化线程池: 创建 num_workers 个 worker 线程.
 * users 指针传递给每个 worker, 用于 handle_http_request.
 * 成功返回 0, 失败返回 -1.
 */
int thread_pool_init(thread_pool_t *pool, int num_workers, UserNode *users);

/*
 * 向任务队列添加一个 client_fd.
 * 队列满时阻塞等待 (生产者等待).
 * 成功返回 0, shutdown 后返回 -1.
 */
int thread_pool_add_task(thread_pool_t *pool, int client_fd);

/*
 * 关闭线程池: 设置 shutdown 标志, 唤醒所有 worker,
 * 等待所有线程退出.
 */
void thread_pool_shutdown(thread_pool_t *pool);

/*
 * 释放线程池资源.
 */
void thread_pool_destroy(thread_pool_t *pool);

#endif
