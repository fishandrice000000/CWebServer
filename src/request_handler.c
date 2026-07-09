#include "request_handler.h"
#include "user_store.h"
#include <stdio.h>
#include <string.h>

#define MAX_LINE 256

/*
 * 请求格式: "GET /<path>"
 * 支持:
 *   GET /hello        → HTTP 200 + "Hello, Web!"
 *   GET /users/<name> → 查 users.csv, 输出 FOUND/NOT_FOUND
 *   其他              → HTTP 404
 */
int handle_request(const char *req_path, const char *out_path, UserNode *users)
{
    FILE *fp_in = fopen(req_path, "r");
    if (fp_in == NULL) {
        fprintf(stderr, "handle_request: cannot open %s\n", req_path);
        return -1;
    }

    char line[MAX_LINE];
    if (fgets(line, sizeof(line), fp_in) == NULL) {
        fclose(fp_in);
        return -1;
    }
    fclose(fp_in);

    /* 去换行 */
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    FILE *fp_out = fopen(out_path, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "handle_request: cannot write %s\n", out_path);
        return -1;
    }

    /* 解析 "GET /xxx" */
    char method[8], path[128];
    if (sscanf(line, "%7s %127s", method, path) != 2) {
        fprintf(fp_out, "HTTP/1.1 400 Bad Request\n");
        fclose(fp_out);
        return -1;
    }

    if (strcmp(path, "/hello") == 0) {
        fprintf(fp_out,
            "HTTP/1.1 200 OK\n"
            "Content-Type: text/html\n"
            "Content-Length: 12\n"
            "\n"
            "Hello, Web!\n");

    } else if (strncmp(path, "/users/", 7) == 0) {
        const char *name = path + 7;

        /* 使用父进程传入的用户链表, 不重复加载 */
        UserNode *p = users->next;
        int found = 0;
        while (p != NULL) {
            if (strcmp(p->data.username, name) == 0) {
                fprintf(fp_out, "FOUND %s %s %s\n",
                        p->data.username, p->data.password, p->data.phone);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (!found) {
            fprintf(fp_out, "NOT_FOUND %s\n", name);
        }

    } else {
        fprintf(fp_out, "HTTP/1.1 404 Not Found\n");
    }

    fclose(fp_out);
    return 0;
}
