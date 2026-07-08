#ifndef USER_INDEX_H
#define USER_INDEX_H

#include "user_store.h"

/*
 * 索引结点: 存 user_t 指针 (不拷贝), 避免重复内存和双 free.
 */
typedef struct user_index_node {
    User                  *user;
    struct user_index_node *left;
    struct user_index_node *right;
} user_index_node_t;

typedef struct {
    user_index_node_t *root;
    int                size;
} user_index_t;

/* 初始化空索引 */
user_index_t *user_index_init(void);

/* 从链表构建 BST 索引 (按 username) */
int user_index_build(user_index_t *index, UserNode *head);

/* 中序遍历输出全部用户 */
void user_index_print(user_index_node_t *root);

/* 按 username 查找, 返回结点指针, 未找到返回 NULL */
user_index_node_t *user_index_find(user_index_node_t *root, const char *name);

/*
 * 对比查找: 分别统计链表和 BST 的查找步数并输出.
 * 步数计数: 每次结点访问 (比较) 计 1.
 */
void user_index_compare(UserNode *head, user_index_node_t *root, const char *name);

/* 释放索引树 (不释放 user 指向的数据, 由 user_store 负责) */
void user_index_destroy(user_index_node_t *root);

#endif
