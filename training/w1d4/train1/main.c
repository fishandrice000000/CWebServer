/*
 * W1D4 训练一：简单 Shell
 *
 * 使用 fork() 创建子进程, execl() 执行命令, waitpid() 等待子进程结束.
 * 支持的命令: ls, pwd, date, whoami, exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD  64
#define MAX_ARGS 8

int main(void)
{
    char input[MAX_CMD];

    while (1) {
        printf("mini-sh> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) break;

        /* 去除换行 */
        char *nl = strchr(input, '\n');
        if (nl) *nl = '\0';
        if (strlen(input) == 0) continue;

        /* 解析命令和参数 */
        char *args[MAX_ARGS];
        int   arg_count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) continue;

        /* 内建命令: exit */
        if (strcmp(args[0], "exit") == 0) {
            printf("bye.\n");
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            /* 子进程: 执行命令 */
            execvp(args[0], args);

            /* 如果 execvp 返回, 说明命令不存在 */
            printf("mini-sh: command not found: %s\n", args[0]);
            exit(1);
        } else {
            /* 父进程: 等待子进程结束 */
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("[exit: %d]\n", WEXITSTATUS(status));
            }
        }
    }

    return 0;
}
