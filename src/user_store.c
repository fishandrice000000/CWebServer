#include "user_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256

/* ---- 工具函数 ---- */

static void remove_newline(char *s)
{
    char *p = strchr(s, '\n');
    if (p) *p = '\0';
    p = strchr(s, '\r');
    if (p) *p = '\0';
}

/*
 * 从 CSV 行中按顺序提取字段, 处理空字段 (连续逗号).
 * 将 line 中的前 max_fields 个字段拷贝到 fields 数组中.
 * 返回实际提取的字段数.
 */
static int parse_csv_line(char *line, char fields[][64], int max_fields)
{
    int   field = 0;
    char *start = line;
    char *p     = line;

    while (field < max_fields) {
        /* 跳过前导空格 */
        while (*start == ' ') start++;

        if (*start == '\0') {
            /* 行尾, 剩余字段置空 */
            fields[field][0] = '\0';
            field++;
            continue;
        }

        /* 找到当前字段的结束位置: 逗号或行尾 */
        p = start;
        while (*p != ',' && *p != '\0') p++;

        /* 复制字段 (去掉尾部空格) */
        int len = p - start;
        while (len > 0 && start[len - 1] == ' ') len--;
        if (len >= 64) len = 63;
        memcpy(fields[field], start, len);
        fields[field][len] = '\0';

        field++;

        if (*p == ',') {
            start = p + 1;
        } else {
            /* 行尾, 剩余字段置空 */
            while (field < max_fields) {
                fields[field][0] = '\0';
                field++;
            }
            break;
        }
    }

    return field;
}

/* ---- 接口实现 ---- */

UserNode *user_store_init(void)
{
    UserNode *head = (UserNode *)malloc(sizeof(UserNode));
    if (head == NULL) {
        fprintf(stderr, "user_store_init: malloc failed\n");
        exit(1);
    }
    head->next = NULL;
    return head;
}

int user_store_load(UserNode *head, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s\n", path);
        return -1;
    }

    char line[MAX_LINE];
    int  first_line = 1;
    int  count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (first_line) {
            first_line = 0;
            continue;
        }
        remove_newline(line);

        if (strlen(line) == 0) continue;  /* 跳过空行 */

        char fields[3][64];
        parse_csv_line(line, fields, 3);

        /* 至少有 username 就认为有效 */
        if (strlen(fields[0]) == 0) continue;

        User u;
        memset(&u, 0, sizeof(u));
        strncpy(u.username, fields[0], sizeof(u.username) - 1);
        strncpy(u.password, fields[1], sizeof(u.password) - 1);
        strncpy(u.phone,    fields[2], sizeof(u.phone)    - 1);

        /* 尾插法 */
        UserNode *s = (UserNode *)malloc(sizeof(UserNode));
        if (s == NULL) {
            fprintf(stderr, "user_store_load: malloc failed\n");
            fclose(fp);
            return -1;
        }
        s->data = u;
        s->next = NULL;

        UserNode *r = head;
        while (r->next != NULL) r = r->next;
        r->next = s;
        count++;
    }

    fclose(fp);
    printf("Loaded %d users from %s\n", count, path);
    return 0;
}

void user_store_list(UserNode *head)
{
    UserNode *p = head->next;
    if (p == NULL) {
        printf("(empty)\n");
        return;
    }
    printf("%-16s %-24s %-16s\n", "USERNAME", "PASSWORD", "PHONE");
    printf("------------------------------------------------------------\n");
    while (p != NULL) {
        printf("%-16s %-24s %-16s\n",
               p->data.username, p->data.password, p->data.phone);
        p = p->next;
    }
}

int user_store_find(UserNode *head, const char *name)
{
    UserNode *p = head->next;
    while (p != NULL) {
        if (strcmp(p->data.username, name) == 0) {
            printf("FOUND\n");
            printf("%-16s %-24s %-16s\n", "USERNAME", "PASSWORD", "PHONE");
            printf("%-16s %-24s %-16s\n",
                   p->data.username, p->data.password, p->data.phone);
            return 0;
        }
        p = p->next;
    }
    printf("NOT_FOUND\n");
    return -1;
}

int user_store_add(UserNode *head, User u)
{
    /* 检查重复 */
    UserNode *p = head->next;
    while (p != NULL) {
        if (strcmp(p->data.username, u.username) == 0) {
            printf("ADD_FAILED (duplicate username)\n");
            return -1;
        }
        p = p->next;
    }

    /* 尾插 */
    UserNode *s = (UserNode *)malloc(sizeof(UserNode));
    if (s == NULL) {
        fprintf(stderr, "user_store_add: malloc failed\n");
        return -1;
    }
    s->data = u;
    s->next = NULL;

    UserNode *r = head;
    while (r->next != NULL) r = r->next;
    r->next = s;
    printf("ADD_OK\n");
    return 0;
}

int user_store_delete(UserNode *head, const char *name)
{
    UserNode *pre = head;
    while (pre->next != NULL) {
        if (strcmp(pre->next->data.username, name) == 0) {
            UserNode *r = pre->next;
            pre->next = r->next;
            free(r);
            printf("DELETE_OK\n");
            return 0;
        }
        pre = pre->next;
    }
    printf("DELETE_FAILED (not found)\n");
    return -1;
}

int user_store_save(UserNode *head, const char *path)
{
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *fp = fopen(tmp_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to write %s\n", tmp_path);
        return -1;
    }

    fprintf(fp, "username,password,phone\n");
    UserNode *p = head->next;
    while (p != NULL) {
        fprintf(fp, "%s,%s,%s\n",
                p->data.username, p->data.password, p->data.phone);
        p = p->next;
    }
    fclose(fp);

    /* 原子替换 */
    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "failed to rename %s → %s\n", tmp_path, path);
        return -1;
    }
    return 0;
}

void user_store_destroy(UserNode *head)
{
    UserNode *p = head;
    while (p != NULL) {
        UserNode *tmp = p;
        p = p->next;
        free(tmp);
    }
}
