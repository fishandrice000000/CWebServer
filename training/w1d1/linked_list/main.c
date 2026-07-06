/*
 * W1D1 训练一：单向链表
 * 实现创建链表、插入结点、遍历结点等基本算法
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct stuInfo {
    char stuName[10];   /* 学生姓名 */
    int  Age;           /* 年龄 */
} ElemType;

typedef struct node {
    ElemType        data;
    struct node    *next;
} ListNode, *ListPtr;

/* 创建链表（头结点） */
ListPtr list_create(void)
{
    ListPtr head = (ListPtr)malloc(sizeof(ListNode));
    if (head == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    head->next = NULL;
    return head;
}

/* 头插法插入结点 */
int list_insert_head(ListPtr head, ElemType data)
{
    ListPtr new_node = (ListPtr)malloc(sizeof(ListNode));
    if (new_node == NULL) {
        return -1;
    }
    new_node->data = data;
    new_node->next = head->next;
    head->next = new_node;
    return 0;
}

/* 尾插法插入结点 */
int list_insert_tail(ListPtr head, ElemType data)
{
    ListPtr new_node = (ListPtr)malloc(sizeof(ListNode));
    if (new_node == NULL) {
        return -1;
    }
    new_node->data = data;
    new_node->next = NULL;

    ListPtr p = head;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = new_node;
    return 0;
}

/* 遍历链表 */
void list_traverse(ListPtr head)
{
    ListPtr p = head->next;
    while (p != NULL) {
        printf("Name: %-10s  Age: %d\n", p->data.stuName, p->data.Age);
        p = p->next;
    }
}

/* 销毁链表 */
void list_destroy(ListPtr head)
{
    ListPtr p = head;
    while (p != NULL) {
        ListPtr tmp = p;
        p = p->next;
        free(tmp);
    }
}

int main(void)
{
    ListPtr list = list_create();

    ElemType s1 = {"Alice", 20};
    ElemType s2 = {"Bob", 21};
    ElemType s3 = {"Charlie", 19};

    list_insert_tail(list, s1);
    list_insert_tail(list, s2);
    list_insert_head(list, s3);

    printf("=== Linked List Traversal ===\n");
    list_traverse(list);

    list_destroy(list);
    return 0;
}
