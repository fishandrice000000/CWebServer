#ifndef USER_STORE_H
#define USER_STORE_H

typedef struct {
    char username[32];
    char password[32];
    char phone[32];
} User;

typedef struct user_node {
    User                data;
    struct user_node   *next;
} UserNode;

/*
 * 初始化带头结点的空用户链表, 返回头结点指针
 */
UserNode *user_store_init(void);

/*
 * 从 CSV 文件加载用户到链表, 跳过标题行.
 * 成功返回 0, 失败返回 -1.
 */
int user_store_load(UserNode *head, const char *path);

/* 遍历输出全部用户 */
void user_store_list(UserNode *head);

/*
 * 按 username 查找, 打印 FOUND 或 NOT_FOUND.
 * 找到返回 0, 未找到返回 -1.
 */
int user_store_find(UserNode *head, const char *name);

/*
 * 添加用户, 若同名则失败.
 * 成功返回 0, 重复返回 -1.
 */
int user_store_add(UserNode *head, User u);

/*
 * 按 username 删除用户.
 * 成功返回 0, 未找到返回 -1.
 */
int user_store_delete(UserNode *head, const char *name);

/* 保存链表到 CSV, 成功返回 0 */
int user_store_save(UserNode *head, const char *path);

/* 销毁链表, 释放全部结点 */
void user_store_destroy(UserNode *head);

#endif
