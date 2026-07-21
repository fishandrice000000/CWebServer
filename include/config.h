#ifndef CONFIG_H
#define CONFIG_H

#define MAX_ROUTES 64

typedef struct {
    char method[16];
    char path[256];
    char handler_name[64];
    char auth[16];
} route_config_t;

typedef struct {
    char host[64];
    int  port;
    char document_root[256];
    char log_level[16];
    char log_path[256];
    route_config_t routes[MAX_ROUTES];
    int  route_count;
} server_config_t;

/*
 * 从 JSON 文件加载配置, 失败打印原因并返回 -1.
 * 成功返回 0.
 */
int  load_config(const char *path, server_config_t *config);
void print_config(const server_config_t *config);

#endif
