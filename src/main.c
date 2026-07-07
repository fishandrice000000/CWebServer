#include "config.h"
#include "http_response.h"
#include "log.h"
#include "user_store.h"
#include <stdio.h>
#include <string.h>

#define USERS_CSV "data/users.csv"

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

        } else {
            printf("usage: %s conf/server.conf {list|find|add|delete} [args...]\n", argv[0]);
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
