/*
 * W1D4 训练二：消费者
 *
 * 从共享缓冲池读取, 写入 consumer<id>.txt 并打印.
 */

#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <consumer_id>\n", argv[0]);
        return 1;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "consumer%s.txt", argv[1]);

    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        perror("fopen consumer file");
        return 1;
    }

    int sem_id = sem_create(SEM_KEY, 1, 1);
    int shm_id = shm_create(SHM_KEY, sizeof(BufferPool));
    BufferPool *pool = (BufferPool *)shm_attach(shm_id);

    int running = 1;
    while (running) {
        int consumed = 0;

        sem_p(sem_id);   /* 进入临界区 */

        for (int i = 0; i < BUFFER_NUM; i++) {
            if (pool->status[i] == 1) {
                char *text = pool->buffer[i];
                printf("[Consumer %s] read from buffer[%d]: %s\n",
                       argv[1], i, text);

                fprintf(fp, "%s\n", text);
                fflush(fp);

                /* 检查终止信号 */
                if (strcmp(text, "END") == 0) {
                    running = 0;
                }

                pool->status[i] = 0;  /* 标记为空闲 */
                consumed = 1;
                break;
            }
        }

        sem_v(sem_id);   /* 离开临界区 */

        if (!consumed) {
            usleep(50000);  /* 池空, 等待生产者生产 */
        }
    }

    fclose(fp);
    shm_detach(pool);

    /* 最后一个消费者清理 IPC */
    shm_del(shm_id);
    sem_del(sem_id);
    return 0;
}
