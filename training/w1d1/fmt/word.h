/*
 * W1D1 训练二：文本格式化 — word 模块
 * 读取一个单词
 */

#ifndef WORD_H
#define WORD_H

/*
 * 从输入中读取下一个单词，存入 word 中（最多 max_len 个字符）。
 * 返回值：成功读取单词的长度，无法读取时返回 0。
 */
int read_word(char *word, int max_len);

#endif
