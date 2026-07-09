#include "process_server.h"
#include "request_handler.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

#define MAX_FILES  128
#define PATH_SZ    1024
#define MSG_SZ     1024

int process_requests(const char *req_dir, const char *out_dir, UserNode *users)
{
    DIR *dir = opendir(req_dir);
    if (dir == NULL) {
        fprintf(stderr, "process_requests: cannot open %s\n", req_dir);
        return -1;
    }

    /* 收集所有 .req 文件 */
    char req_files[MAX_FILES][PATH_SZ];
    int  total = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".req") != NULL) {
            snprintf(req_files[total], PATH_SZ,
                     "%s/%s", req_dir, entry->d_name);
            total++;
        }
    }
    closedir(dir);

    char buf[MSG_SZ];
    snprintf(buf, sizeof(buf), "process_server: found %d request(s)", total);
    log_info(buf);

    /* Bug 1 修复: fork 前刷新所有 stdio 缓冲区 */
    fflush(stdout);
    fflush(stderr);
    fflush(NULL);  /* 刷新所有流 */

    /* fork 子进程处理每个请求 */
    pid_t pids[MAX_FILES];
    int   child_count = 0;

    for (int i = 0; i < total; i++) {
        /* 构造输出路径: xxx.req → xxx.out */
        char out_path[PATH_SZ];
        const char *basename = strrchr(req_files[i], '/');
        basename = (basename != NULL) ? basename + 1 : req_files[i];

        char tmp[PATH_SZ];
        strncpy(tmp, basename, PATH_SZ - 1);
        tmp[PATH_SZ - 1] = '\0';
        char *dot = strstr(tmp, ".req");
        if (dot) *dot = '\0';
        snprintf(out_path, PATH_SZ, "%.256s/%.256s.out", out_dir, tmp);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            /* ---- 子进程 ---- */
            char child_msg[MSG_SZ];
            snprintf(child_msg, MSG_SZ,
                     "child[%d]: processing %.512s", getpid(), req_files[i]);
            log_info(child_msg);

            handle_request(req_files[i], out_path, users);

            snprintf(child_msg, MSG_SZ,
                     "child[%d]: done -> %.512s", getpid(), out_path);
            log_info(child_msg);

            /* 子进程必须 exit, 不能 return */
            exit(0);
        } else {
            /* ---- 父进程 ---- */
            pids[child_count++] = pid;
        }
    }

    /* 父进程等待所有子进程结束 */
    for (int i = 0; i < child_count; i++) {
        int status;
        pid_t ret = waitpid(pids[i], &status, 0);
        char wait_msg[MSG_SZ];
        if (ret > 0) {
            snprintf(wait_msg, MSG_SZ,
                     "parent: child %d exited (status %d)", ret,
                     WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        } else {
            snprintf(wait_msg, MSG_SZ,
                     "parent: waitpid for %d failed", pids[i]);
        }
        log_info(wait_msg);
    }

    log_info("process_server: all done");
    return 0;
}
