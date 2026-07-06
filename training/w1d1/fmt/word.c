/*
 * 读取一个单词的实现
 */

#include <stdio.h>
#include <ctype.h>
#include "word.h"

int read_word(char *word, int max_len)
{
    int ch;
    int len = 0;

    /* 跳过前导空白字符 */
    while ((ch = getchar()) != EOF && isspace(ch))
        ;

    if (ch == EOF) {
        return 0;
    }

    /* 读取单词字符 */
    do {
        if (len < max_len - 1) {
            word[len++] = ch;
        }
    } while ((ch = getchar()) != EOF && !isspace(ch));

    word[len] = '\0';
    return len;
}
