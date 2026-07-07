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

        User u;
        memset(&u, 0, sizeof(u));

        char *token = strtok(line, ",");
        int   field = 0;

        while (token != NULL && field < 3) {
            switch (field) {
            case 0: strncpy(u.username, token, sizeof(u.username) - 1); break;
            case 1: strncpy(u.password, token, sizeof(u.password) - 1); break;
            case 2: strncpy(u.phone,    token, sizeof(u.phone)    - 1); break;
            }
            field++;
            token = strtok(NULL, ",");
        }

        /* 至少有 username 就认为有效 */
        if (strlen(u.username) > 0) {
            /* 尾插法 */
            UserNode *s = (UserNode *)malloc(sizeof(UserNode));
            s->data = u;
            s->next = NULL;

            UserNode *r = head;
            while (r->next != NULL) r = r->next;
            r->next = s;
            count++;
        }
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
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to write %s\n", path);
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
