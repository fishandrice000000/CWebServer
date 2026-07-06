/*
 * W1D1 训练二：文本格式化 — 主程序
 * 用法：fmt < sourcefile > newfile
 *
 * 功能：
 *   1) 删除额外的空格和空行
 *   2) 对每行文字进行填充和调整（两端对齐）
 *   3) 最后一行不调整
 */

#include <stdio.h>
#include "word.h"
#include "line.h"

#define MAX_LINE_WIDTH  60
#define MAX_WORD_LEN    128

int main(void)
{
    char word[MAX_WORD_LEN];

    line_clear();

    for (;;) {
        int len = read_word(word, MAX_WORD_LEN);
        if (len == 0) {
            /* 无法读取更多单词，输出最后一行（不调整） */
            line_print(0, MAX_LINE_WIDTH);
            break;
        }

        if (line_is_full(word, MAX_LINE_WIDTH)) {
            /* 行已满，输出当前行（进行调整） */
            line_print(1, MAX_LINE_WIDTH);
            line_clear();
        }

        line_add_word(word);
    }

    return 0;
}
