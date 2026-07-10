/*
 * W1D5 训练：哲学家进餐问题
 *
 * 两种模式:
 *   lock    — pthread_mutex_lock (阻塞加锁, 可能死锁)
 *   trylock — pthread_mutex_trylock + 让权等待 (预防死锁)
 *
 * 用法: ./philosopher [num] [mode]
 *   num:  哲学家数量 (默认 5)
 *   mode: lock | trylock (默认 lock)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_NUM 5

/* ---- 全局数据 ---- */

pthread_mutex_t *chopsticks = NULL;
int              philosopher_count;
int              running = 1;
int              use_trylock = 0;

/* ---- 筷子操作 ---- */

static void take_chopstick(int id, int idx)
{
    if (use_trylock) {
        /* 非阻塞加锁: 拿不到就立即放下已有的, 让权等待 */
        if (pthread_mutex_trylock(&chopsticks[idx]) == 0) {
            return;  /* 成功 */
        }
        /* 失败: 释放已拿到的筷子, 稍后重试 */
        int other = (id % 2 == 0) ? (id + 1) % philosopher_count
                                  : id;
        pthread_mutex_unlock(&chopsticks[other]);
        usleep(10000);
        /* 重试: 这次阻塞等待 */
        pthread_mutex_lock(&chopsticks[other]);
        pthread_mutex_lock(&chopsticks[idx]);
    } else {
        /* 阻塞加锁 */
        pthread_mutex_lock(&chopsticks[idx]);
    }
}

static void put_chopstick(int idx)
{
    pthread_mutex_unlock(&chopsticks[idx]);
}

/* ---- 哲学家线程 ---- */

void *philosopher(void *arg)
{
    int id = *(int *)arg;
    free(arg);

    int left  = id;
    int right = (id + 1) % philosopher_count;

    /*
     * lock 模式: 所有人先左后右 (经典死锁场景)
     * trylock 模式: 偶数先左后右, 奇数先右后左 (预防死锁)
     */
    int first, second;
    if (use_trylock) {
        first  = (id % 2 == 0) ? left  : right;
        second = (id % 2 == 0) ? right : left;
    } else {
        first  = left;
        second = right;
    }

    while (running) {
        printf("Philosopher %d is thinking.\n", id);
        usleep((rand() % 1000 + 500) * 1000);  /* 思考 0.5-1.5s */

        printf("Philosopher %d is hungry.\n", id);

        /* 拿第一只筷子 */
        if (use_trylock) {
            if (pthread_mutex_trylock(&chopsticks[first]) != 0) {
                printf("Philosopher %d: cannot get chopstick %d, retry.\n",
                       id, first);
                usleep(50000);
                continue;
            }
        } else {
            pthread_mutex_lock(&chopsticks[first]);
        }

        printf("Philosopher %d picked up chopstick %d.\n", id, first);

        /* 拿第二只筷子 */
        take_chopstick(id, second);

        printf("Philosopher %d picked up chopstick %d.\n", id, second);
        printf("Philosopher %d is eating.\n", id);
        usleep((rand() % 1000 + 500) * 1000);  /* 进餐 0.5-1.5s */

        put_chopstick(first);
        put_chopstick(second);
    }
    return NULL;
}

/* ---- 主程序 ---- */

int main(int argc, char *argv[])
{
    philosopher_count = (argc >= 2) ? atoi(argv[1]) : DEFAULT_NUM;
    if (philosopher_count < 2) philosopher_count = 2;
    if (philosopher_count > 20) philosopher_count = 20;

    if (argc >= 3 && strcmp(argv[2], "trylock") == 0) {
        use_trylock = 1;
    }

    printf("Philosopher Dining: %d philosophers, mode=%s\n",
           philosopher_count, use_trylock ? "trylock" : "lock");
    printf("Press Ctrl+C to stop.\n\n");

    /* 初始化筷子互斥量 */
    chopsticks = malloc(sizeof(pthread_mutex_t) * philosopher_count);
    for (int i = 0; i < philosopher_count; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
    }

    /* 创建哲学家线程 */
    pthread_t *threads = malloc(sizeof(pthread_t) * philosopher_count);
    for (int i = 0; i < philosopher_count; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, philosopher, id);
    }

    /* 运行一段时间让用户观察死锁/正常情况 */
    sleep(30);
    running = 0;

    /* 等待所有线程结束 */
    for (int i = 0; i < philosopher_count; i++) {
        pthread_join(threads[i], NULL);
    }

    /* 清理 */
    for (int i = 0; i < philosopher_count; i++) {
        pthread_mutex_destroy(&chopsticks[i]);
    }
    free(chopsticks);
    free(threads);

    printf("All philosophers done.\n");
    return 0;
}
