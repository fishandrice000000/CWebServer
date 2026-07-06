/*
 * W1D1 训练二：文本格式化 — line 模块
 * 行缓冲区操作
 */

#ifndef LINE_H
#define LINE_H

/* 初始化行缓冲区 */
void line_clear(void);

/* 向行缓冲区添加一个单词 */
void line_add_word(const char *word);

/* 判断行缓冲区是否已满（再加入下一个单词会超出最大宽度） */
int  line_is_full(const char *next_word, int max_width);

/* 输出行缓冲区内容，justify 为 1 时进行调整（两端对齐），为 0 时不调整 */
void line_print(int justify, int max_width);

#endif
