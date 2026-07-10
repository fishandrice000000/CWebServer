#ifndef THREAD_SERVER_H
#define THREAD_SERVER_H

#include <pthread.h>
#include "user_store.h"

int thread_server_run(const char *req_dir, const char *out_dir,
                      UserNode *users, int num_workers);

#endif
