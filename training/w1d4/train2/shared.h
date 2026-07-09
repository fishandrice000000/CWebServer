#ifndef SHARED_H
#define SHARED_H

#include <stddef.h>

#define SHM_KEY   0x1a0a
#define SEM_KEY   0x1a0b
#define BUFFER_NUM 5
#define BUFFER_SZ  100
#define TEXT_SZ    100

/* 缓冲池: 5 个缓冲区 + 状态数组 (0=空闲, 1=已使用) */
typedef struct {
    char buffer[BUFFER_NUM][BUFFER_SZ];
    int  status[BUFFER_NUM];        /* 0: 可生产, 1: 可消费 */
} BufferPool;

/* 信号量集: sem[0] 互斥信号量 */
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

int   sem_create(int key, int nsems, int init_val);
int   sem_p(int sem_id);
int   sem_v(int sem_id);
void  sem_del(int sem_id);

int   shm_create(int key, size_t size);
void *shm_attach(int shmid);
int   shm_detach(void *addr);
void  shm_del(int shmid);

#endif
