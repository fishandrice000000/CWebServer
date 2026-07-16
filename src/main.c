#include "config.h"
#include "http_response.h"
#include "log.h"
#include "user_store.h"
#include "user_index.h"
#include "process_server.h"
#include "thread_server.h"
#include "tcp_server.h"
#include "tcp_fork_server.h"
#include "tcp_pool_server.h"
#include "epoll_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USERS_CSV       "data/users.csv"
#define USERS_LARGE_CSV "data/users_large.csv"

int main(int argc, char *argv[])
{
    server_config_t config;
    char            response[256];

    if (argc < 2) {
        printf("usage: %s conf/server.conf [command ...]\n", argv[0]);
        return 1;
    }

    memset(&config, 0, sizeof(config));

    if (load_config(argv[1], &config) != 0) {
        printf("failed to load config\n");
        return 1;
    }

    if (log_init(config.log_path) != 0) {
        printf("failed to open log file\n");
        return 1;
    }
    log_info("server config loaded");

    print_config(&config);

    /* ---- V0.2: 用户管理子命令 ---- */
    if (argc >= 3) {
        UserNode *users = user_store_init();
        user_store_load(users, USERS_CSV);

        const char *cmd = argv[2];

        if (strcmp(cmd, "list") == 0) {
            user_store_list(users);

        } else if (strcmp(cmd, "find") == 0 && argc >= 4) {
            user_store_find(users, argv[3]);

        } else if (strcmp(cmd, "add") == 0 && argc >= 6) {
            User u;
            memset(&u, 0, sizeof(u));
            strncpy(u.username, argv[3], sizeof(u.username) - 1);
            strncpy(u.password, argv[4], sizeof(u.password) - 1);
            strncpy(u.phone,    argv[5], sizeof(u.phone)    - 1);
            user_store_add(users, u);
            user_store_save(users, USERS_CSV);

        } else if (strcmp(cmd, "delete") == 0 && argc >= 4) {
            user_store_delete(users, argv[3]);
            user_store_save(users, USERS_CSV);

        /* ---- V0.3: BST 索引子命令 ---- */
        } else if (strcmp(cmd, "index") == 0) {
            user_index_t *idx = user_index_init();
            user_index_build(idx, users);
            user_index_print(idx->root);
            user_index_destroy(idx->root);
            free(idx);

        } else if (strcmp(cmd, "find-index") == 0 && argc >= 4) {
            user_index_t *idx = user_index_init();
            user_index_build(idx, users);
            user_index_node_t *found = user_index_find(idx->root, argv[3]);
            if (found) {
                printf("FOUND\n");
                printf("%-16s %-24s %-16s\n", "USERNAME", "PASSWORD", "PHONE");
                printf("%-16s %-24s %-16s\n",
                       found->user->username, found->user->password, found->user->phone);
            } else {
                printf("NOT_FOUND\n");
            }
            user_index_destroy(idx->root);
            free(idx);

        } else if (strcmp(cmd, "compare") == 0 && argc >= 4) {
            user_index_t *idx = user_index_init();
            user_index_build(idx, users);
            user_index_compare(users, idx->root, argv[3]);
            user_index_destroy(idx->root);
            free(idx);

        /* ---- V0.4: 多进程请求处理 ---- */
        } else if (strcmp(cmd, "process") == 0) {
            log_info("process_server: starting");
            process_requests("requests", "outputs", users);
            log_info("process_server: finished");

        /* ---- V0.5: 多线程请求处理 ---- */
        } else if (strcmp(cmd, "threaded") == 0) {
            int num_workers = (argc >= 4) ? atoi(argv[3]) : 3;
            if (num_workers < 1) num_workers = 1;
            log_info("thread_server: starting");
            thread_server_run("requests", "outputs", users, num_workers);
            log_info("thread_server: finished");

        /* ---- V0.6: TCP 网络服务 (单连接) ---- */
        } else if (strcmp(cmd, "serve") == 0) {
            log_info("tcp_server: starting");
            tcp_server_run(config.host, config.port, users);
            log_info("tcp_server: finished");

        /* ---- V0.7: 多进程 TCP 网络服务 ---- */
        } else if (strcmp(cmd, "serve-fork") == 0) {
            int max_clients = (argc >= 4) ? atoi(argv[3]) : 0;
            if (max_clients < 0) max_clients = 0;
            log_info("tcp_fork_server: starting");
            tcp_fork_server_run(config.host, config.port, users,
                                max_clients);
            log_info("tcp_fork_server: finished");

        /* ---- V0.8: 线程池 TCP 网络服务 ---- */
        } else if (strcmp(cmd, "serve-pool") == 0) {
            int num_workers = (argc >= 4) ? atoi(argv[3]) : 3;
            int max_clients = (argc >= 5) ? atoi(argv[4]) : 0;
            if (num_workers < 1)  num_workers  = 1;
            if (max_clients < 0)  max_clients  = 0;
            log_info("tcp_pool_server: starting");
            tcp_pool_server_run(config.host, config.port, users,
                                num_workers, max_clients);
            log_info("tcp_pool_server: finished");

        /* ---- V1.0: epoll HTTP 服务器 ---- */
        } else if (strcmp(cmd, "serve-epoll") == 0) {
            int max_requests = (argc >= 4) ? atoi(argv[3]) : 0;
            if (max_requests < 0) max_requests = 0;
            log_info("epoll_server: starting");
            epoll_server_run(config.host, config.port, users,
                             max_requests);
            log_info("epoll_server: finished");

        } else {
            printf("usage: %s conf/server.conf {list|find|add|delete|index|find-index|compare|process|serve|serve-fork|serve-pool|serve-epoll} [args...]\n", argv[0]);
        }

        user_store_destroy(users);
        log_close();
        return 0;
    }

    /* ---- V0.1: HTTP 响应 ---- */
    log_info("document root loaded");

    if (build_hello_response(response, sizeof(response)) < 0) {
        log_error("failed to build response");
        log_close();
        return 1;
    }

    printf("%s", response);

    log_close();
    return 0;
}
