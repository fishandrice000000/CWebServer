/*
 * W1D4 训练二：生产者
 *
 * 从 producer<id>.txt 读取一行内容, 写入共享缓冲池的空闲缓冲区.
 */

#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <producer_id>\n", argv[0]);
        return 1;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "producer%s.txt", argv[1]);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen producer file");
        return 1;
    }

    int sem_id = sem_create(SEM_KEY, 1, 1);   /* 互斥信号量, 初值 1 */
    int shm_id = shm_create(SHM_KEY, sizeof(BufferPool));
    BufferPool *pool = (BufferPool *)shm_attach(shm_id);

    char line[TEXT_SZ];
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* 去换行 */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        int found = 0;
        while (!found) {
            sem_p(sem_id);   /* 进入临界区 */

            /* 找空闲缓冲区 */
            for (int i = 0; i < BUFFER_NUM; i++) {
                if (pool->status[i] == 0) {
                    strncpy(pool->buffer[i], line, BUFFER_SZ - 1);
                    pool->buffer[i][BUFFER_SZ - 1] = '\0';
                    pool->status[i] = 1;   /* 标记为已使用 */
                    printf("[Producer %s] wrote to buffer[%d]: %s\n",
                           argv[1], i, line);
                    found = 1;
                    break;
                }
            }

            sem_v(sem_id);   /* 离开临界区 */

            if (!found) {
                usleep(50000);  /* 池满, 等待消费者消费 */
            }
        }
    }

    fclose(fp);
    shm_detach(pool);
    /* 不删除 IPC 对象, 留给 consumer 清理 */
    return 0;
}
