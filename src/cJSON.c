#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

static const char *ep;

static int cJSON_strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d) return d;
        s1++; s2++;
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

/* ---- 跳过空白 ---- */
static const char *skip(const char *in) {
    while (in && *in && (unsigned char)*in <= 32) in++;
    return in;
}

/* ---- 解析内部函数 ---- */
static cJSON *cJSON_New_Item(void)
{
    cJSON *node = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (node) memset(node, 0, sizeof(cJSON));
    return node;
}

static char *parse_string(const char *str)
{
    if (*str != '\"') { ep = str; return NULL; }
    str++;
    const char *ptr = str;
    int len = 0;
    /* 计算长度 */
    while (*ptr != '\"' && *ptr) {
        if (*ptr == '\\') { ptr++; if (*ptr) ptr++; }
        else ptr++;
        len++;
    }
    char *out = (char *)cJSON_malloc((size_t)len + 1);
    if (!out) return NULL;
    char *o = out;
    ptr = str;
    while (*ptr != '\"' && *ptr) {
        if (*ptr == '\\') {
            ptr++;
            switch (*ptr) {
                case '\"': *o++ = '\"'; break;
                case '\\': *o++ = '\\'; break;
                case '/':  *o++ = '/';  break;
                case 'b':  *o++ = '\b'; break;
                case 'f':  *o++ = '\f'; break;
                case 'n':  *o++ = '\n'; break;
                case 'r':  *o++ = '\r'; break;
                case 't':  *o++ = '\t'; break;
                case 'u':  *o++ = '?'; ptr += 4; break; /* unicode → ? */
                default:   *o++ = *ptr; break;
            }
        } else {
            *o++ = *ptr;
        }
        ptr++;
    }
    *o = '\0';
    if (*ptr == '\"') ptr++;
    ep = ptr;
    return out;
}

static cJSON *parse_value(const char *value);

static cJSON *parse_object(const char *value)
{
    if (*value != '{') { ep = value; return NULL; }
    cJSON *obj = cJSON_New_Item();
    if (!obj) return NULL;
    obj->type = cJSON_Object;

    value = skip(value + 1);
    if (*value == '}') { ep = value + 1; return obj; }

    cJSON *prev = NULL;
    while (*value) {
        value = skip(value);
        if (*value != '\"') { cJSON_Delete(obj); return NULL; }
        char *key = parse_string(value);
        if (!key) { cJSON_Delete(obj); return NULL; }
        value = skip(ep);

        if (*value != ':') { cJSON_free(key); cJSON_Delete(obj); return NULL; }
        value = skip(value + 1);

        cJSON *item = parse_value(value);
        if (!item) { cJSON_free(key); cJSON_Delete(obj); return NULL; }
        value = skip(ep);

        item->string = key;
        /* link into child list */
        if (!obj->child) obj->child = item;
        if (prev) prev->next = item;
        item->prev = prev;
        prev = item;

        if (*value == '}') { ep = value + 1; return obj; }
        if (*value != ',') { cJSON_Delete(obj); return NULL; }
        value++;
    }
    cJSON_Delete(obj);
    return NULL;
}

static cJSON *parse_array(const char *value)
{
    if (*value != '[') { ep = value; return NULL; }
    cJSON *arr = cJSON_New_Item();
    if (!arr) return NULL;
    arr->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']') { ep = value + 1; return arr; }

    cJSON *prev = NULL;
    while (*value) {
        value = skip(value);
        cJSON *item = parse_value(value);
        if (!item) { cJSON_Delete(arr); return NULL; }
        value = skip(ep);
        if (!arr->child) arr->child = item;
        if (prev) prev->next = item;
        item->prev = prev;
        prev = item;
        if (*value == ']') { ep = value + 1; return arr; }
        if (*value != ',') { cJSON_Delete(arr); return NULL; }
        value++;
    }
    cJSON_Delete(arr);
    return NULL;
}

static cJSON *parse_value(const char *value)
{
    if (!value) return NULL;
    value = skip(value);

    switch (*value) {
    case '\"': {
        cJSON *item = cJSON_New_Item();
        if (!item) return NULL;
        item->type = cJSON_String;
        item->valuestring = parse_string(value);
        if (!item->valuestring) { cJSON_free(item); return NULL; }
        return item;
    }
    case '{':  return parse_object(value);
    case '[':  return parse_array(value);
    case 't':
        if (strncmp(value, "true", 4) == 0) {
            cJSON *item = cJSON_New_Item();
            if (item) { item->type = cJSON_True; ep = value + 4; }
            return item;
        }
        break;
    case 'f':
        if (strncmp(value, "false", 5) == 0) {
            cJSON *item = cJSON_New_Item();
            if (item) { item->type = cJSON_False; ep = value + 5; }
            return item;
        }
        break;
    case 'n':
        if (strncmp(value, "null", 4) == 0) {
            cJSON *item = cJSON_New_Item();
            if (item) { item->type = cJSON_NULL; ep = value + 4; }
            return item;
        }
        break;
    default:
        if ((*value >= '0' && *value <= '9') || *value == '-') {
            cJSON *item = cJSON_New_Item();
            if (!item) return NULL;
            item->type = cJSON_Number;
            item->valuedouble = strtod(value, (char **)&ep);
            item->valueint = (int)item->valuedouble;
            return item;
        }
        break;
    }
    ep = value;
    return NULL;
}

/* ---- 公开 API ---- */

cJSON *cJSON_Parse(const char *value)
{
    if (!value) return NULL;
    cJSON *c = parse_value(value);
    return c;
}

cJSON *cJSON_GetObjectItem(const cJSON *obj, const char *string)
{
    if (!obj || !string) return NULL;
    cJSON *c = obj->child;
    while (c) {
        if (c->string && cJSON_strcasecmp(c->string, string) == 0)
            return c;
        c = c->next;
    }
    return NULL;
}

int cJSON_GetArraySize(const cJSON *array)
{
    if (!array) return 0;
    int count = 0;
    cJSON *c = array->child;
    while (c) { count++; c = c->next; }
    return count;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item)
{
    if (!array) return NULL;
    cJSON *c = array->child;
    while (c && item > 0) { c = c->next; item--; }
    return c;
}

void cJSON_Delete(cJSON *c)
{
    if (!c) return;
    cJSON *next;
    while (c) {
        next = c->next;
        if (c->child) cJSON_Delete(c->child);
        if (c->valuestring) cJSON_free(c->valuestring);
        if (c->string) cJSON_free(c->string);
        cJSON_free(c);
        c = next;
    }
}
