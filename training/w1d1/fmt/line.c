/*
 * 行缓冲区操作的实现
 */

#include <stdio.h>
#include <string.h>
#include "line.h"

#define MAX_LINE_WORDS 256
#define MAX_WORD_LEN   128

static char line_words[MAX_LINE_WORDS][MAX_WORD_LEN];
static int  line_word_count = 0;
static int  line_char_count  = 0;

void line_clear(void)
{
    line_word_count = 0;
    line_char_count  = 0;
}

void line_add_word(const char *word)
{
    if (line_word_count >= MAX_LINE_WORDS) {
        return;
    }
    strncpy(line_words[line_word_count], word, MAX_WORD_LEN - 1);
    line_words[line_word_count][MAX_WORD_LEN - 1] = '\0';

    /* 如果不是第一个单词，前面有一个空格 */
    if (line_word_count > 0) {
        line_char_count++;
    }
    line_char_count += strlen(word);
    line_word_count++;
}

int line_is_full(const char *next_word, int max_width)
{
    int next_len = strlen(next_word);
    int extra    = (line_word_count > 0) ? 1 : 0;  /* 单词前的空格 */
    return (line_char_count + extra + next_len) > max_width;
}

void line_print(int justify, int max_width)
{
    if (line_word_count == 0) {
        return;
    }

    if (!justify || line_word_count == 1) {
        /* 不调整：直接输出，单词间一个空格 */
        for (int i = 0; i < line_word_count; i++) {
            if (i > 0) putchar(' ');
            printf("%s", line_words[i]);
        }
        putchar('\n');
        return;
    }

    /*
     * 两端对齐：将多余的空格均匀分配到单词之间
     */
    int total_spaces = max_width - line_char_count;
    int gaps         = line_word_count - 1;

    for (int i = 0; i < line_word_count; i++) {
        printf("%s", line_words[i]);
        if (i < gaps) {
            int spaces = total_spaces / gaps;
            if (i < total_spaces % gaps) {
                spaces++;
            }
            for (int s = 0; s < spaces + 1; s++) {
                putchar(' ');
            }
        }
    }
    putchar('\n');
}
