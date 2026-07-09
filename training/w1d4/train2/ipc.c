#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* ---- 信号量 ---- */

int sem_create(int key, int nsems, int init_val)
{
    int sem_id;

    /* 尝试以 IPC_EXCL 创建, 失败说明已存在 */
    sem_id = semget((key_t)key, nsems, IPC_CREAT | IPC_EXCL | 0666);
    if (sem_id >= 0) {
        /* 我们是创建者, 设置初始值 */
        union semun sem_union;
        sem_union.val = init_val;
        semctl(sem_id, 0, SETVAL, sem_union);
        printf("[init] created semaphore (id=%d)\n", sem_id);
    } else {
        /* 已存在, 直接获取 */
        sem_id = semget((key_t)key, nsems, 0666);
        if (sem_id == -1) {
            perror("semget");
            exit(1);
        }
    }
    return sem_id;
}

int sem_p(int sem_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op  = -1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        perror("semop P");
        return -1;
    }
    return 0;
}

int sem_v(int sem_id)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op  = 1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        perror("semop V");
        return -1;
    }
    return 0;
}

void sem_del(int sem_id)
{
    union semun sem_union;
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        perror("semctl IPC_RMID");
}

/* ---- 共享内存 ---- */

int shm_create(int key, size_t size)
{
    int shm_id;

    /* 尝试以 IPC_EXCL 创建, 失败说明已存在 */
    shm_id = shmget((key_t)key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id >= 0) {
        printf("[init] created shared memory (id=%d)\n", shm_id);
    } else {
        /* 已存在, 直接获取 */
        shm_id = shmget((key_t)key, size, 0666);
        if (shm_id == -1) {
            perror("shmget");
            exit(1);
        }
    }
    return shm_id;
}

void *shm_attach(int shmid)
{
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    return addr;
}

int shm_detach(void *addr)
{
    if (shmdt(addr) == -1) {
        perror("shmdt");
        return -1;
    }
    return 0;
}

void shm_del(int shmid)
{
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        perror("shmctl IPC_RMID");
}
