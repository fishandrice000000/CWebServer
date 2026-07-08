/*
 * W1D3 训练一：二叉排序树 (BST) 练习
 *
 * 功能：
 *   1) 从 contacts.csv 读取联系人 (name / phone)
 *   2) 以 name 为关键字创建 BST
 *   3) 实现查找和新增
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256

/* ---- 数据结构 ---- */

typedef struct {
    char name[20];
    char phone[20];
} Contact;

typedef struct TreeNode {
    Contact          contact;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

/* ---- BST 操作 ---- */

TreeNode *create_node(Contact contact)
{
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    node->contact = contact;
    node->left  = NULL;
    node->right = NULL;
    return node;
}

TreeNode *insert_node(TreeNode *root, Contact contact)
{
    if (root == NULL) {
        return create_node(contact);
    }
    int cmp = strcmp(contact.name, root->contact.name);
    if (cmp < 0) {
        root->left = insert_node(root->left, contact);
    } else if (cmp > 0) {
        root->right = insert_node(root->right, contact);
    }
    /* cmp == 0: 同名跳过 */
    return root;
}

TreeNode *search_node(TreeNode *root, const char *name)
{
    if (root == NULL) return NULL;

    int cmp = strcmp(name, root->contact.name);
    if (cmp == 0) return root;
    if (cmp < 0)  return search_node(root->left, name);
    return search_node(root->right, name);
}

void inorder_traversal(TreeNode *root)
{
    if (root != NULL) {
        inorder_traversal(root->left);
        printf("%-16s %-16s\n", root->contact.name, root->contact.phone);
        inorder_traversal(root->right);
    }
}

void destroy_tree(TreeNode *root)
{
    if (root != NULL) {
        destroy_tree(root->left);
        destroy_tree(root->right);
        free(root);
    }
}

/* ---- CSV 读取 ---- */

static void remove_newline(char *s)
{
    char *p = strchr(s, '\n');
    if (p) *p = '\0';
}

TreeNode *load_from_csv(const char *filename)
{
    TreeNode *root = NULL;
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open %s\n", filename);
        return NULL;
    }

    char line[MAX_LINE];
    int  first_line = 1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (first_line) { first_line = 0; continue; }
        remove_newline(line);
        if (strlen(line) == 0) continue;

        Contact c;
        memset(&c, 0, sizeof(c));
        char *name  = strtok(line, ",");
        char *phone = strtok(NULL, ",");
        if (name)  strncpy(c.name,  name,  sizeof(c.name)  - 1);
        if (phone) strncpy(c.phone, phone, sizeof(c.phone) - 1);

        root = insert_node(root, c);
    }

    fclose(fp);
    return root;
}

/* ---- 主程序 ---- */

int main(void)
{
    TreeNode *root = load_from_csv("contacts.csv");
    if (root == NULL) return 1;

    printf("=== All contacts (inorder) ===\n");
    inorder_traversal(root);

    /* 查找 */
    printf("\n=== Search 'zhangsan' ===\n");
    TreeNode *found = search_node(root, "zhangsan");
    if (found)
        printf("Found: %s, %s\n", found->contact.name, found->contact.phone);
    else
        printf("Not found.\n");

    printf("\n=== Search 'nobody' ===\n");
    found = search_node(root, "nobody");
    if (found)
        printf("Found: %s, %s\n", found->contact.name, found->contact.phone);
    else
        printf("Not found.\n");

    /* 新增 */
    printf("\n=== Add 'testuser' ===\n");
    Contact new_contact;
    memset(&new_contact, 0, sizeof(new_contact));
    strncpy(new_contact.name,  "testuser",  sizeof(new_contact.name)  - 1);
    strncpy(new_contact.phone, "13900000001", sizeof(new_contact.phone) - 1);
    root = insert_node(root, new_contact);

    printf("After add, inorder:\n");
    inorder_traversal(root);

    destroy_tree(root);
    return 0;
}
