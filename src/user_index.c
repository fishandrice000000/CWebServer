#include "user_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- 内部辅助 ---- */

static user_index_node_t *create_index_node(User *user)
{
    user_index_node_t *node = (user_index_node_t *)malloc(sizeof(user_index_node_t));
    if (node == NULL) {
        fprintf(stderr, "create_index_node: malloc failed\n");
        exit(1);
    }
    node->user  = user;
    node->left  = NULL;
    node->right = NULL;
    return node;
}

static user_index_node_t *insert_index_node(user_index_node_t *root, User *user)
{
    if (root == NULL) return create_index_node(user);

    int cmp = strcmp(user->username, root->user->username);
    if (cmp < 0)
        root->left = insert_index_node(root->left, user);
    else if (cmp > 0)
        root->right = insert_index_node(root->right, user);
    /* cmp == 0: 同名跳过 */
    return root;
}

/* ---- 接口实现 ---- */

user_index_t *user_index_init(void)
{
    user_index_t *index = (user_index_t *)malloc(sizeof(user_index_t));
    if (index == NULL) {
        fprintf(stderr, "user_index_init: malloc failed\n");
        exit(1);
    }
    index->root = NULL;
    index->size = 0;
    return index;
}

int user_index_build(user_index_t *index, UserNode *head)
{
    UserNode *p = head->next;
    int count = 0;

    while (p != NULL) {
        index->root = insert_index_node(index->root, &p->data);
        count++;
        p = p->next;
    }
    index->size = count;
    printf("Index built: %d users\n", count);
    return 0;
}

void user_index_print(user_index_node_t *root)
{
    if (root == NULL) return;
    user_index_print(root->left);
    printf("%-16s %-24s %-16s\n",
           root->user->username, root->user->password, root->user->phone);
    user_index_print(root->right);
}

user_index_node_t *user_index_find(user_index_node_t *root, const char *name)
{
    if (root == NULL) return NULL;

    int cmp = strcmp(name, root->user->username);
    if (cmp == 0) return root;
    if (cmp < 0)  return user_index_find(root->left, name);
    return user_index_find(root->right, name);
}

void user_index_compare(UserNode *head, user_index_node_t *root, const char *name)
{
    /* 链表查找 */
    int  list_steps = 0;
    UserNode *p = head->next;
    int  list_found = 0;

    while (p != NULL) {
        list_steps++;
        if (strcmp(p->data.username, name) == 0) {
            list_found = 1;
            break;
        }
        p = p->next;
    }

    /* BST 查找 */
    int tree_steps = 0;
    int tree_found = 0;
    user_index_node_t *q = root;

    while (q != NULL) {
        tree_steps++;
        int cmp = strcmp(name, q->user->username);
        if (cmp == 0) {
            tree_found = 1;
            break;
        }
        q = (cmp < 0) ? q->left : q->right;
    }

    printf("Compare '%s':\n", name);
    printf("  list search: %d steps -> %s\n", list_steps,
           list_found ? "FOUND" : "NOT_FOUND");
    printf("  tree search:  %d steps -> %s\n", tree_steps,
           tree_found ? "FOUND" : "NOT_FOUND");
}

void user_index_destroy(user_index_node_t *root)
{
    if (root == NULL) return;
    user_index_destroy(root->left);
    user_index_destroy(root->right);
    free(root);
}
