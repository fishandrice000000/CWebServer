/*
 * W1D2 训练二：有序链表
 *
 * 原始 CSV 中数据是乱序的, 需要：
 *   1) 按 id 递增建立有序链表
 *   2) 实现增删改查（插入时保持有序）
 *   3) 按 username 递增对链表排序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE   256
#define CSV_FILE   "users.csv"

/* ---- 数据结构 ---- */

typedef struct {
    int   id;
    char  username[32];
    char  password[32];
    char  contact[32];
} User;

typedef struct node {
    User          data;
    struct node  *next;
} Node, *LinkList;

/* ---- 工具函数 ---- */

static void remove_newline(char *s)
{
    char *p = strchr(s, '\n');
    if (p) *p = '\0';
    p = strchr(s, '\r');
    if (p) *p = '\0';
}

/* ---- 链表操作 ---- */

/* 初始化带头结点的空链表 */
LinkList list_init(void)
{
    LinkList L = (LinkList)malloc(sizeof(Node));
    L->next = NULL;
    return L;
}

/* 有序插入: 按 id 递增, 如果 id 已存在则返回 -1 */
int list_insert_ordered(LinkList L, User user)
{
    Node *s = (Node *)malloc(sizeof(Node));
    s->data = user;

    /* 找插入位置的前驱: pre 之后的位置 */
    Node *pre = L;
    while (pre->next != NULL && pre->next->data.id < user.id) {
        pre = pre->next;
    }

    if (pre->next != NULL && pre->next->data.id == user.id) {
        free(s);
        return -1;  /* 重复 id */
    }

    s->next = pre->next;
    pre->next = s;
    return 0;
}

/* 按 id 删除 */
int list_delete_by_id(LinkList L, int id)
{
    Node *pre = L;
    while (pre->next != NULL && pre->next->data.id != id) {
        pre = pre->next;
    }
    if (pre->next == NULL) return -1;

    Node *r = pre->next;
    pre->next = r->next;
    free(r);
    return 0;
}

/* 按 id 查找 */
Node *list_find_by_id(LinkList L, int id)
{
    Node *p = L->next;
    while (p != NULL && p->data.id != id) {
        p = p->next;
    }
    return p;
}

/* 遍历 */
void list_traverse(LinkList L)
{
    Node *p = L->next;
    printf("%-6s %-12s %-12s %-15s\n", "ID", "Username", "Password", "Contact");
    printf("------------------------------------------------------\n");
    while (p != NULL) {
        printf("%-6d %-12s %-12s %-15s\n",
               p->data.id, p->data.username,
               p->data.password, p->data.contact);
        p = p->next;
    }
}

/* 销毁 */
void list_destroy(LinkList L)
{
    Node *p = L;
    while (p != NULL) {
        Node *t = p;
        p = p->next;
        free(t);
    }
}

/* ---- 排序: 按 username 递增 (冒泡, 交换数据) ---- */
void list_sort_by_username(LinkList L)
{
    if (L->next == NULL) return;

    Node *i, *j;
    User  tmp;

    for (i = L->next; i->next != NULL; i = i->next) {
        for (j = i->next; j != NULL; j = j->next) {
            if (strcmp(i->data.username, j->data.username) > 0) {
                tmp = i->data;
                i->data = j->data;
                j->data = tmp;
            }
        }
    }
}

/* ---- CSV 读写 ---- */

int load_from_csv_ordered(LinkList L, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s\n", filename);
        return -1;
    }

    char line[MAX_LINE];
    int  first_line = 1, count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (first_line) { first_line = 0; continue; }

        remove_newline(line);
        User u;
        memset(&u, 0, sizeof(u));

        char *token = strtok(line, ",");
        int   field = 0;
        while (token != NULL) {
            switch (field) {
            case 0: u.id = atoi(token); break;
            case 1: strncpy(u.username, token, sizeof(u.username) - 1); break;
            case 2: strncpy(u.password, token, sizeof(u.password) - 1); break;
            case 3: strncpy(u.contact,  token, sizeof(u.contact)  - 1); break;
            }
            field++;
            token = strtok(NULL, ",");
        }
        if (field >= 3) {
            list_insert_ordered(L, u);
            count++;
        }
    }

    fclose(fp);
    printf("Loaded %d users (ordered by id).\n", count);
    return 0;
}

int save_to_csv(LinkList L, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) return -1;

    fprintf(fp, "id,username,password,contact\n");
    Node *p = L->next;
    while (p != NULL) {
        fprintf(fp, "%d,%s,%s,%s\n",
                p->data.id, p->data.username,
                p->data.password, p->data.contact);
        p = p->next;
    }
    fclose(fp);
    return 0;
}

/* ---- 菜单 ---- */

static void menu_add(LinkList L)
{
    User u;
    memset(&u, 0, sizeof(u));
    printf("Enter id: ");     scanf("%d", &u.id); getchar();
    printf("Enter username: "); fgets(u.username, sizeof(u.username), stdin);
    remove_newline(u.username);
    printf("Enter password: "); fgets(u.password, sizeof(u.password), stdin);
    remove_newline(u.password);
    printf("Enter contact: ");  fgets(u.contact, sizeof(u.contact), stdin);
    remove_newline(u.contact);

    if (list_insert_ordered(L, u) == 0) {
        printf("User added (ordered by id).\n");
    } else {
        printf("Duplicate id, not added.\n");
    }
}

static void menu_delete(LinkList L)
{
    int id;
    printf("Enter id to delete: "); scanf("%d", &id);
    if (list_delete_by_id(L, id) == 0)
        printf("Deleted.\n");
    else
        printf("Not found.\n");
}

static void menu_modify(LinkList L)
{
    int id;
    printf("Enter id to modify: "); scanf("%d", &id); getchar();

    Node *p = list_find_by_id(L, id);
    if (p == NULL) { printf("Not found.\n"); return; }

    char buf[32];
    printf("New username (%s): ", p->data.username);
    fgets(buf, sizeof(buf), stdin); remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.username, buf, sizeof(p->data.username) - 1);

    printf("New password (%s): ", p->data.password);
    fgets(buf, sizeof(buf), stdin); remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.password, buf, sizeof(p->data.password) - 1);

    printf("New contact (%s): ", p->data.contact);
    fgets(buf, sizeof(buf), stdin); remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.contact, buf, sizeof(p->data.contact) - 1);

    printf("Modified.\n");
}

static void menu_search(LinkList L)
{
    int id;
    printf("Enter id: "); scanf("%d", &id);
    Node *p = list_find_by_id(L, id);
    if (p) printf("Found: %d, %s, %s, %s\n", p->data.id, p->data.username, p->data.password, p->data.contact);
    else   printf("Not found.\n");
}

int main(void)
{
    LinkList L = list_init();
    load_from_csv_ordered(L, CSV_FILE);
    list_traverse(L);

    int choice;
    do {
        printf("\n--- Ordered List Menu ---\n");
        printf("1. Show all (by id)\n");
        printf("2. Add user (ordered)\n");
        printf("3. Delete user\n");
        printf("4. Modify user\n");
        printf("5. Search user\n");
        printf("6. Sort by username\n");
        printf("7. Save & Exit\n");
        printf("8. Exit without save\n");
        printf("Choice: "); scanf("%d", &choice); getchar();

        switch (choice) {
        case 1: list_traverse(L);              break;
        case 2: menu_add(L);                   break;
        case 3: menu_delete(L);                break;
        case 4: menu_modify(L);                break;
        case 5: menu_search(L);                break;
        case 6: list_sort_by_username(L);
                printf("Sorted by username:\n");
                list_traverse(L);              break;
        case 7: save_to_csv(L, CSV_FILE);
                printf("Saved.\n");            break;
        case 8: printf("Exit.\n");             break;
        default: printf("Invalid.\n");         break;
        }
    } while (choice != 7 && choice != 8);

    list_destroy(L);
    return 0;
}
