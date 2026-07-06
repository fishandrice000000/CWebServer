#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void remove_newline(char *text)
{
    char *p = strchr(text, '\n');
    if (p) *p = '\0';
}

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
        } else if (strcmp(key, "host") == 0) {
            strncpy(config->host, value, sizeof(config->host) - 1);
        } else if (strcmp(key, "port") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "root") == 0) {
            strncpy(config->root, value, sizeof(config->root) - 1);
        } else if (strcmp(key, "log") == 0) {
            strncpy(config->log_path, value, sizeof(config->log_path) - 1);
        }
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
