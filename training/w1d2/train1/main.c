/*
 * W1D2 训练一：CSV 文件 + 结构体链表 CRUD
 *
 * 功能：
 *   1) 从 users.csv 读取用户信息, 以链表方式加载到内存
 *   2) 实现增、删、改、查
 *   3) 将修改后的数据写回 users.csv
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

/* ---- 链表基本操作 ---- */

/* 初始化带头结点的空链表 */
LinkList list_init(void)
{
    LinkList L = (LinkList)malloc(sizeof(Node));
    if (L == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    L->next = NULL;
    return L;
}

/* 尾插法添加结点 */
void list_append(LinkList L, User user)
{
    Node *s = (Node *)malloc(sizeof(Node));
    s->data = user;
    s->next = NULL;

    Node *r = L;
    while (r->next != NULL) {
        r = r->next;
    }
    r->next = s;
}

/* 按 id 查找结点, 返回前驱指针 (pre->next 为目标结点) */
static Node *find_prev(LinkList L, int id)
{
    Node *pre = L;
    while (pre->next != NULL && pre->next->data.id != id) {
        pre = pre->next;
    }
    return pre;
}

/* 删除指定 id 的用户, 成功返回 0, 失败返回 -1 */
int list_delete(LinkList L, int id)
{
    Node *pre = find_prev(L, id);
    if (pre->next == NULL) {
        return -1;  /* 未找到 */
    }
    Node *r = pre->next;
    pre->next = r->next;
    free(r);
    return 0;
}

/* 按 id 查找用户, 返回结点指针, 未找到返回 NULL */
Node *list_find_by_id(LinkList L, int id)
{
    Node *pre = find_prev(L, id);
    return pre->next;
}

/* 遍历链表 */
void list_traverse(LinkList L)
{
    Node *p = L->next;
    printf("%-6s %-12s %-12s %-15s\n", "ID", "Username", "Password", "Contact");
    printf("------------------------------------------------------\n");
    if (p == NULL) {
        printf("(empty)\n");
        return;
    }
    while (p != NULL) {
        printf("%-6d %-12s %-12s %-15s\n",
               p->data.id, p->data.username,
               p->data.password, p->data.contact);
        p = p->next;
    }
}

/* 销毁链表 */
void list_destroy(LinkList L)
{
    Node *p = L;
    while (p != NULL) {
        Node *tmp = p;
        p = p->next;
        free(tmp);
    }
}

/* ---- CSV 文件读写 ---- */

/* 去除行尾换行符 */
static void remove_newline(char *s)
{
    char *p = strchr(s, '\n');
    if (p) *p = '\0';
    p = strchr(s, '\r');
    if (p) *p = '\0';
}

/* 从 CSV 加载用户到链表, 跳过标题行 */
int load_from_csv(LinkList L, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s\n", filename);
        return -1;
    }

    char line[MAX_LINE];
    int  first_line = 1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (first_line) {
            first_line = 0;   /* 跳过标题行 */
            continue;
        }
        remove_newline(line);

        User u;
        memset(&u, 0, sizeof(u));

        char *token;
        int   field = 0;

        token = strtok(line, ",");
        while (token != NULL) {
            switch (field) {
            case 0: u.id = atoi(token);                  break;
            case 1: strncpy(u.username, token, sizeof(u.username) - 1); break;
            case 2: strncpy(u.password, token, sizeof(u.password) - 1); break;
            case 3: strncpy(u.contact,  token, sizeof(u.contact)  - 1); break;
            }
            field++;
            token = strtok(NULL, ",");
        }

        if (field >= 3) {  /* 至少有三个字段才认为有效 */
            list_append(L, u);
        }
    }

    fclose(fp);
    return 0;
}

/* 将链表数据写回 CSV */
int save_to_csv(LinkList L, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s for writing\n", filename);
        return -1;
    }

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

/* ---- 交互式菜单 ---- */

static void menu_add(LinkList L)
{
    User u;
    memset(&u, 0, sizeof(u));
    printf("Enter id: ");
    scanf("%d", &u.id);
    getchar();
    printf("Enter username: ");
    fgets(u.username, sizeof(u.username), stdin);
    remove_newline(u.username);
    printf("Enter password: ");
    fgets(u.password, sizeof(u.password), stdin);
    remove_newline(u.password);
    printf("Enter contact: ");
    fgets(u.contact, sizeof(u.contact), stdin);
    remove_newline(u.contact);

    list_append(L, u);
    printf("User added.\n");
}

static void menu_delete(LinkList L)
{
    int id;
    printf("Enter id to delete: ");
    scanf("%d", &id);
    if (list_delete(L, id) == 0) {
        printf("User %d deleted.\n", id);
    } else {
        printf("User %d not found.\n", id);
    }
}

static void menu_modify(LinkList L)
{
    int id;
    printf("Enter id to modify: ");
    scanf("%d", &id);
    getchar();

    Node *p = list_find_by_id(L, id);
    if (p == NULL) {
        printf("User %d not found.\n", id);
        return;
    }

    printf("Enter new username (%s): ", p->data.username);
    char buf[32];
    fgets(buf, sizeof(buf), stdin);
    remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.username, buf, sizeof(p->data.username) - 1);

    printf("Enter new password (%s): ", p->data.password);
    fgets(buf, sizeof(buf), stdin);
    remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.password, buf, sizeof(p->data.password) - 1);

    printf("Enter new contact (%s): ", p->data.contact);
    fgets(buf, sizeof(buf), stdin);
    remove_newline(buf);
    if (strlen(buf) > 0) strncpy(p->data.contact, buf, sizeof(p->data.contact) - 1);

    printf("User %d modified.\n", id);
}

static void menu_search(LinkList L)
{
    int id;
    printf("Enter id to search: ");
    scanf("%d", &id);

    Node *p = list_find_by_id(L, id);
    if (p == NULL) {
        printf("User %d not found.\n", id);
        return;
    }
    printf("Found: %d, %s, %s, %s\n",
           p->data.id, p->data.username,
           p->data.password, p->data.contact);
}

int main(void)
{
    LinkList L = list_init();

    if (load_from_csv(L, CSV_FILE) != 0) {
        list_destroy(L);
        return 1;
    }
    printf("Loaded users from %s:\n", CSV_FILE);
    list_traverse(L);

    int choice;
    do {
        printf("\n--- Menu ---\n");
        printf("1. Show all\n");
        printf("2. Add user\n");
        printf("3. Delete user\n");
        printf("4. Modify user\n");
        printf("5. Search user\n");
        printf("6. Save & Exit\n");
        printf("7. Exit without save\n");
        printf("Choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
        case 1: list_traverse(L);                        break;
        case 2: menu_add(L);                             break;
        case 3: menu_delete(L);                          break;
        case 4: menu_modify(L);                          break;
        case 5: menu_search(L);                          break;
        case 6: save_to_csv(L, CSV_FILE);
                printf("Saved.\n");                      break;
        case 7: printf("Exit without save.\n");          break;
        default: printf("Invalid choice.\n");            break;
        }
    } while (choice != 6 && choice != 7);

    list_destroy(L);
    return 0;
}
