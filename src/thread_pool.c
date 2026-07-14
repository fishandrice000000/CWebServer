/*
 * thread_pool.c — 线程池实现 (V0.8)
 *
 * 固定大小的 worker 线程池 + 循环队列任务缓冲.
 * 主线程 (生产者) 通过 thread_pool_add_task 放入 client_fd,
 * worker 线程 (消费者) 取出并调用 handle_http_request 处理.
 * 使用 mutex + cond 实现生产-消费同步.
 */

#include "thread_pool.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 传递给每个 worker 线程的上下文 */
typedef struct {
    thread_pool_t *pool;
    int            id;
} worker_ctx_t;

/* ---- worker 线程函数 ---- */
static void *worker_func(void *arg)
{
    worker_ctx_t  *ctx  = (worker_ctx_t *)arg;
    thread_pool_t *pool = ctx->pool;
    int            id   = ctx->id;

    /* ctx 的生命周期由线程自己管理, 串起来后 free */
    free(ctx);

    while (1)
    {
        pthread_mutex_lock(&pool->queue.mutex);

        /* 队列为空且未 shutdown → 等待 */
        while (pool->queue.count == 0 && !pool->queue.shutdown)
            pthread_cond_wait(&pool->queue.not_empty, &pool->queue.mutex);

        /* shutdown 且队列空 → 退出 */
        if (pool->queue.shutdown && pool->queue.count == 0)
        {
            pthread_mutex_unlock(&pool->queue.mutex);
            break;
        }

        /* 取出一个任务 */
        int fd = pool->queue.client_fds[pool->queue.head];
        pool->queue.head = (pool->queue.head + 1) % MAX_QUEUE;
        pool->queue.count--;

        /* 向可能阻塞的生产者发信号: 队列现在有空位了 */
        pthread_cond_signal(&pool->queue.not_full);
        pthread_mutex_unlock(&pool->queue.mutex);

        /* 处理 HTTP 请求 */
        {
            char msg[64];
            snprintf(msg, sizeof(msg),
                     "tcp_pool_server: worker %d handling fd=%d", id, fd);
            log_info(msg);
        }

        handle_http_request(fd, pool->users);
        close(fd);
    }

    {
        char msg[48];
        snprintf(msg, sizeof(msg),
                 "tcp_pool_server: worker %d exiting", id);
        log_info(msg);
    }
    return NULL;
}

/* ---- 初始化 ---- */
int thread_pool_init(thread_pool_t *pool, int num_workers, UserNode *users)
{
    if (num_workers < 1) num_workers = 1;

    memset(pool, 0, sizeof(*pool));
    pool->num_workers = num_workers;
    pool->users       = users;

    pool->queue.head  = 0;
    pool->queue.tail  = 0;
    pool->queue.count = 0;
    pool->queue.shutdown = 0;

    if (pthread_mutex_init(&pool->queue.mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        return -1;
    }
    if (pthread_cond_init(&pool->queue.not_empty, NULL) != 0)
    {
        perror("pthread_cond_init");
        pthread_mutex_destroy(&pool->queue.mutex);
        return -1;
    }
    if (pthread_cond_init(&pool->queue.not_full, NULL) != 0)
    {
        perror("pthread_cond_init");
        pthread_cond_destroy(&pool->queue.not_empty);
        pthread_mutex_destroy(&pool->queue.mutex);
        return -1;
    }

    pool->workers = malloc(sizeof(pthread_t) * num_workers);
    if (pool->workers == NULL)
    {
        perror("malloc");
        pthread_cond_destroy(&pool->queue.not_full);
        pthread_cond_destroy(&pool->queue.not_empty);
        pthread_mutex_destroy(&pool->queue.mutex);
        return -1;
    }

    for (int i = 0; i < num_workers; i++)
    {
        worker_ctx_t *ctx = malloc(sizeof(worker_ctx_t));
        if (ctx == NULL)
        {
            perror("malloc");
            /* 清理已创建的线程和资源 */
            pool->queue.shutdown = 1;
            pthread_cond_broadcast(&pool->queue.not_empty);
            pthread_cond_broadcast(&pool->queue.not_full);
            for (int j = 0; j < i; j++)
                pthread_join(pool->workers[j], NULL);
            free(pool->workers);
            pthread_cond_destroy(&pool->queue.not_full);
            pthread_cond_destroy(&pool->queue.not_empty);
            pthread_mutex_destroy(&pool->queue.mutex);
            return -1;
        }
        ctx->pool = pool;
        ctx->id   = i;

        if (pthread_create(&pool->workers[i], NULL, worker_func, ctx) != 0)
        {
            perror("pthread_create");
            free(ctx);
            pool->queue.shutdown = 1;
            pthread_cond_broadcast(&pool->queue.not_empty);
            pthread_cond_broadcast(&pool->queue.not_full);
            for (int j = 0; j < i; j++)
                pthread_join(pool->workers[j], NULL);
            free(pool->workers);
            pthread_cond_destroy(&pool->queue.not_full);
            pthread_cond_destroy(&pool->queue.not_empty);
            pthread_mutex_destroy(&pool->queue.mutex);
            return -1;
        }
    }

    return 0;
}

/* ---- 添加任务 (生产者) ---- */
int thread_pool_add_task(thread_pool_t *pool, int client_fd)
{
    pthread_mutex_lock(&pool->queue.mutex);

    /* shutdown 后不再接受新任务 */
    if (pool->queue.shutdown)
    {
        pthread_mutex_unlock(&pool->queue.mutex);
        return -1;
    }

    /* 队列满时阻塞等待 (使用 while 防虚假唤醒) */
    while (pool->queue.count >= MAX_QUEUE && !pool->queue.shutdown)
        pthread_cond_wait(&pool->queue.not_full, &pool->queue.mutex);

    if (pool->queue.shutdown)
    {
        pthread_mutex_unlock(&pool->queue.mutex);
        return -1;
    }

    pool->queue.client_fds[pool->queue.tail] = client_fd;
    pool->queue.tail = (pool->queue.tail + 1) % MAX_QUEUE;
    pool->queue.count++;

    /* 唤醒一个等待的 worker */
    pthread_cond_signal(&pool->queue.not_empty);
    pthread_mutex_unlock(&pool->queue.mutex);

    return 0;
}

/* ---- 关闭线程池 ---- */
void thread_pool_shutdown(thread_pool_t *pool)
{
    pthread_mutex_lock(&pool->queue.mutex);
    pool->queue.shutdown = 1;
    /* 唤醒所有阻塞的 worker 和生产者 */
    pthread_cond_broadcast(&pool->queue.not_empty);
    pthread_cond_broadcast(&pool->queue.not_full);
    pthread_mutex_unlock(&pool->queue.mutex);

    /* 等待所有 worker 退出 */
    for (int i = 0; i < pool->num_workers; i++)
        pthread_join(pool->workers[i], NULL);
}

/* ---- 销毁 ---- */
void thread_pool_destroy(thread_pool_t *pool)
{
    free(pool->workers);
    pthread_cond_destroy(&pool->queue.not_full);
    pthread_cond_destroy(&pool->queue.not_empty);
    pthread_mutex_destroy(&pool->queue.mutex);
    memset(pool, 0, sizeof(*pool));
}
