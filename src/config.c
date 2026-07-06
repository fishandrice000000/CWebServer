#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void remove_newline(char *text) {【todo】}

int load_config(const char *path, server_config_t *config) {
    FILE *fp = fopen(path, "r");
    char  line[256];

    if (fp == NULL || config == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *key;
        char *value;

        remove_newline(line);
        key   = strtok(line, "=");
        value = strtok(NULL, "=");

        if (strcmp(key, "server_name") == 0) {
            strncpy(config->server_name, value, sizeof(config->server_name) - 1);
        } else if (strcmp(key, "host") == 0) {【todo】
        } else if (strcmp(key, "port") == 0) {【todo】
        } else if (strcmp(key, "root") == 0) {【todo】
        } else if (strcmp(key, "log") == 0) {【todo】}
    }

    fclose(fp);
    return 0;
}

void print_config(const server_config_t *config) {
    printf("server_name=%s\n", config->server_name);
    printf("host=%s\n", config->host);
    printf("port=%d\n", config->port);
    printf("root=%s\n", config->root);
    printf("log=%s\n", config->log_path);
}
