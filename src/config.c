#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(const char *path, server_config_t *config)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "config: cannot open %s\n", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (buf == NULL) { fclose(fp); return -1; }
    fread(buf, 1, (size_t)sz, fp);
    buf[sz] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (root == NULL) {
        fprintf(stderr, "config: JSON parse error\n");
        return -1;
    }

    memset(config, 0, sizeof(*config));

    /* ---- server ---- */
    cJSON *server = cJSON_GetObjectItem(root, "server");
    if (!server) {
        fprintf(stderr, "config: missing 'server' section\n");
        cJSON_Delete(root); return -1;
    }
    cJSON *h = cJSON_GetObjectItem(server, "host");
    cJSON *p = cJSON_GetObjectItem(server, "port");
    cJSON *d = cJSON_GetObjectItem(server, "document_root");
    if (!h || !p || !d) {
        fprintf(stderr, "config: server needs host, port, document_root\n");
        cJSON_Delete(root); return -1;
    }
    strncpy(config->host, h->valuestring, sizeof(config->host)-1);
    config->port = p->valueint;
    strncpy(config->document_root, d->valuestring,
            sizeof(config->document_root)-1);
    if (config->port < 1 || config->port > 65535) {
        fprintf(stderr, "config: invalid port %d\n", config->port);
        cJSON_Delete(root); return -1;
    }

    /* ---- logging ---- */
    cJSON *log = cJSON_GetObjectItem(root, "logging");
    strcpy(config->log_level, "INFO");
    strcpy(config->log_path, "./logs/server.log");
    if (log) {
        cJSON *lv = cJSON_GetObjectItem(log, "level");
        cJSON *lf = cJSON_GetObjectItem(log, "file");
        if (lv && lv->valuestring) {
            const char *ls = lv->valuestring;
            if (strcmp(ls, "DEBUG") && strcmp(ls, "INFO") &&
                strcmp(ls, "WARN") && strcmp(ls, "ERROR")) {
                fprintf(stderr, "config: invalid log level '%s'\n", ls);
                cJSON_Delete(root); return -1;
            }
            strncpy(config->log_level, ls, sizeof(config->log_level)-1);
        }
        if (lf && lf->valuestring)
            strncpy(config->log_path, lf->valuestring,
                    sizeof(config->log_path)-1);
    }

    /* ---- routes ---- */
    cJSON *routes = cJSON_GetObjectItem(root, "routes");
    if (routes && routes->type == cJSON_Array) {
        int n = cJSON_GetArraySize(routes);
        for (int i = 0; i < n && i < MAX_ROUTES; i++) {
            cJSON *r = cJSON_GetArrayItem(routes, i);
            if (!r) continue;
            cJSON *m = cJSON_GetObjectItem(r, "method");
            cJSON *pt = cJSON_GetObjectItem(r, "path");
            cJSON *hn = cJSON_GetObjectItem(r, "handler");
            if (!m || !pt || !hn) {
                fprintf(stderr, "config: route[%d] missing field\n", i);
                cJSON_Delete(root); return -1;
            }
            const char *mt = m->valuestring;
            if (mt) {
                if (strcmp(mt, "GET") && strcmp(mt, "POST") &&
                    strcmp(mt, "PUT") && strcmp(mt, "DELETE")) {
                    fprintf(stderr, "config: invalid method '%s'\n", mt);
                    cJSON_Delete(root); return -1;
                }
                strncpy(config->routes[i].method, mt,
                        sizeof(config->routes[i].method)-1);
            }
            const char *pv = pt->valuestring;
            if (!pv || pv[0] != '/') {
                fprintf(stderr, "config: path must start with '/'\n");
                cJSON_Delete(root); return -1;
            }
            strncpy(config->routes[i].path, pv,
                    sizeof(config->routes[i].path)-1);
            /* 查重 */
            for (int j = 0; j < i; j++)
                if (strcmp(config->routes[j].method,
                           config->routes[i].method)==0 &&
                    strcmp(config->routes[j].path,
                           config->routes[i].path)==0) {
                    fprintf(stderr, "config: duplicate route %s %s\n",
                            mt, pv);
                    cJSON_Delete(root); return -1;
                }
            if (hn->valuestring)
                strncpy(config->routes[i].handler_name, hn->valuestring,
                        sizeof(config->routes[i].handler_name)-1);
        }
        config->route_count = (n < MAX_ROUTES) ? n : MAX_ROUTES;
    }

    cJSON_Delete(root);
    return 0;
}

void print_config(const server_config_t *config)
{
    printf("host=%s\n", config->host);
    printf("port=%d\n", config->port);
    printf("root=%s\n", config->document_root);
    printf("log=%s\n", config->log_path);
    for (int i = 0; i < config->route_count; i++)
        printf("route: %s %s → %s\n",
               config->routes[i].method,
               config->routes[i].path,
               config->routes[i].handler_name);
}
