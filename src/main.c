#include "config.h"
#include "http_response.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    server_config_t config;
    char            response[64];

    if (argc != 2) {
        printf("usage: %s conf/server.conf\n", argv[0]);
        return 1;
    }

    memset(&config, 0, sizeof(config));

    if (load_config(argv[1], &config) != 0) {
        printf("failed to load config\n");
        return 1;
    }

    log_init(config.log_path);
    log_info("server config loaded");
    log_info("document root loaded");

    print_config(&config);

    if (build_hello_response(response, sizeof(response)) < 0) {
        log_error("failed to build response");
        log_close();
        return 1;
    }

    printf("%s", response);

    log_close();
    return 0;
}
