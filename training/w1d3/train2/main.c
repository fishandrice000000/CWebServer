/*
 * W1D3 训练二：二叉排序树 (BST) 练习
 *
 * 功能：
 *   1) 从 contacts.csv 读取联系人, 以 name 为关键字创建 BST
 *   2) 实现查找, 新增, 删除 (叶子/单子树/双子树三种情况), 修改
 *   3) 中序遍历保存到 CSV
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

static TreeNode *create_node(Contact contact)
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
    if (root == NULL) return create_node(contact);

    int cmp = strcmp(contact.name, root->contact.name);
    if (cmp < 0)
        root->left = insert_node(root->left, contact);
    else if (cmp > 0)
        root->right = insert_node(root->right, contact);
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

/*
 * 删除三种情况:
 *   1) 叶子: 直接删
 *   2) 只有左或右子树: 用子树替换
 *   3) 有左右子树: 用右子树最小结点 (中序后继) 替换
 */
TreeNode *delete_node(TreeNode *root, const char *name)
{
    if (root == NULL) return NULL;

    int cmp = strcmp(name, root->contact.name);
    if (cmp < 0) {
        root->left = delete_node(root->left, name);
    } else if (cmp > 0) {
        root->right = delete_node(root->right, name);
    } else {
        /* 情况 1 & 2: 无左或右子树 */
        if (root->left == NULL) {
            TreeNode *temp = root->right;
            free(root);
            return temp;
        }
        if (root->right == NULL) {
            TreeNode *temp = root->left;
            free(root);
            return temp;
        }
        /* 情况 3: 找右子树最小结点 (中序后继) */
        TreeNode *successor = root->right;
        while (successor->left != NULL)
            successor = successor->left;

        root->contact = successor->contact;
        root->right = delete_node(root->right, successor->contact.name);
    }
    return root;
}

void inorder_traversal(TreeNode *root)
{
    if (root != NULL) {
        inorder_traversal(root->left);
        printf("%-16s %-16s\n", root->contact.name, root->contact.phone);
        inorder_traversal(root->right);
    }
}

void inorder_save(TreeNode *root, FILE *fp)
{
    if (root != NULL) {
        inorder_save(root->left, fp);
        fprintf(fp, "%s,%s\n", root->contact.name, root->contact.phone);
        inorder_save(root->right, fp);
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

/* ---- CSV 读写 ---- */

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

int save_to_csv(TreeNode *root, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to write %s\n", filename);
        return -1;
    }
    fprintf(fp, "name,phone\n");
    inorder_save(root, fp);
    fclose(fp);
    printf("Saved to %s\n", filename);
    return 0;
}

/* ---- 主程序 ---- */

int main(void)
{
    TreeNode *root = load_from_csv("contacts.csv");
    if (root == NULL) return 1;

    printf("=== All contacts (inorder) ===\n");
    inorder_traversal(root);

    /* 查找 */
    printf("\n=== Find 'lisi' ===\n");
    TreeNode *f = search_node(root, "lisi");
    printf(f ? "Found: %s, %s\n" : "Not found.\n",
           f ? f->contact.name : "", f ? f->contact.phone : "");

    /* 修改 */
    printf("\n=== Modify 'lisi' phone ===\n");
    f = search_node(root, "lisi");
    if (f) {
        strncpy(f->contact.phone, "99999999999", sizeof(f->contact.phone) - 1);
        printf("Modified: %s, %s\n", f->contact.name, f->contact.phone);
    }

    /* 删除叶子结点 */
    printf("\n=== Delete leaf 'zhengshi' ===\n");
    root = delete_node(root, "zhengshi");
    inorder_traversal(root);

    /* 删除只有单子树的结点 */
    printf("\n=== Delete 'lisi' (has child 'zhangsan' ?) ===\n");
    root = delete_node(root, "lisi");
    inorder_traversal(root);

    /* 删除有双子树的结点 */
    printf("\n=== Delete 'wangwu' (has both children) ===\n");
    root = delete_node(root, "wangwu");
    inorder_traversal(root);

    /* 保存 */
    printf("\n=== Save ===\n");
    save_to_csv(root, "contacts_result.csv");

    destroy_tree(root);
    return 0;
}
