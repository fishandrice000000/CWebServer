#include "thread_server.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#define MAX_TASKS   128
#define PATH_SZ     1024

typedef struct {
    char tasks[MAX_TASKS][PATH_SZ];
    int  front;
    int  count;
    int  total_processed;
    int  done;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} request_queue_t;

static request_queue_t queue;

typedef struct {
    const char *out_dir;
    UserNode   *users;
    int         id;
} worker_arg_t;

static void *worker_thread(void *arg)
{
    worker_arg_t *warg = (worker_arg_t *)arg;
    char msg[128];

    snprintf(msg, sizeof(msg), "worker[%d]: started", warg->id);
    log_info(msg);

    while (1) {
        pthread_mutex_lock(&queue.mutex);

        while (queue.count == 0 && !queue.done)
            pthread_cond_wait(&queue.cond, &queue.mutex);

        if (queue.count == 0 && queue.done) {
            pthread_mutex_unlock(&queue.mutex);
            break;
        }

        char req_path[PATH_SZ];
        strncpy(req_path, queue.tasks[queue.front], PATH_SZ - 1);
        req_path[PATH_SZ - 1] = '\0';
        queue.front = (queue.front + 1) % MAX_TASKS;
        queue.count--;
        pthread_mutex_unlock(&queue.mutex);

        char out_path[PATH_SZ];
        const char *base = strrchr(req_path, '/');
        base = (base != NULL) ? base + 1 : req_path;
        char tmp[PATH_SZ];
        strncpy(tmp, base, PATH_SZ - 1);
        tmp[PATH_SZ - 1] = '\0';
        char *dot = strstr(tmp, ".req");
        if (dot) *dot = '\0';
        snprintf(out_path, PATH_SZ, "%.500s/%.500s.out", warg->out_dir, tmp);

        handle_request(req_path, out_path, warg->users);

        pthread_mutex_lock(&queue.mutex);
        queue.total_processed++;
        pthread_mutex_unlock(&queue.mutex);
    }

    snprintf(msg, sizeof(msg), "worker[%d]: exiting", warg->id);
    log_info(msg);
    free(warg);
    return NULL;
}

int thread_server_run(const char *req_dir, const char *out_dir,
                      UserNode *users, int num_workers)
{
    memset(&queue, 0, sizeof(queue));

    DIR *dir = opendir(req_dir);
    if (dir == NULL) {
        fprintf(stderr, "thread_server: cannot open %s\n", req_dir);
        return -1;
    }

    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.cond, NULL);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".req") != NULL) {
            pthread_mutex_lock(&queue.mutex);
            int rear = (queue.front + queue.count) % MAX_TASKS;
            snprintf(queue.tasks[rear], PATH_SZ, "%.512s/%.512s",
                     req_dir, entry->d_name);
            queue.count++;
            pthread_cond_signal(&queue.cond);
            pthread_mutex_unlock(&queue.mutex);
        }
    }
    closedir(dir);

    char buf[64];
    snprintf(buf, sizeof(buf), "thread_server: enqueued %d request(s)",
             queue.count);
    log_info(buf);

    pthread_mutex_lock(&queue.mutex);
    queue.done = 1;
    pthread_cond_broadcast(&queue.cond);
    pthread_mutex_unlock(&queue.mutex);

    pthread_t *workers = malloc(sizeof(pthread_t) * num_workers);
    if (workers == NULL) {
        fprintf(stderr, "thread_server: malloc workers failed\n");
        pthread_mutex_destroy(&queue.mutex);
        pthread_cond_destroy(&queue.cond);
        return -1;
    }

    for (int i = 0; i < num_workers; i++) {
        worker_arg_t *warg = malloc(sizeof(worker_arg_t));
        if (warg == NULL) {
            fprintf(stderr, "thread_server: malloc warg failed\n");
            free(workers);
            pthread_mutex_destroy(&queue.mutex);
            pthread_cond_destroy(&queue.cond);
            return -1;
        }
        warg->out_dir = out_dir;
        warg->users   = users;
        warg->id      = i;
        pthread_create(&workers[i], NULL, worker_thread, warg);
    }

    for (int i = 0; i < num_workers; i++)
        pthread_join(workers[i], NULL);

    snprintf(buf, sizeof(buf), "thread_server: all done, %d processed",
             queue.total_processed);
    log_info(buf);

    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.cond);
    free(workers);

    return 0;
}
