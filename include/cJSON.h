/*
 * cJSON — 轻量 JSON 解析库 (MIT License)
 * 原版: https://github.com/DaveGamble/cJSON
 * 为 V1.4 精简: 仅保留解析和读取功能
 */
#ifndef CJSON_H
#define CJSON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cJSON {
    struct cJSON *next, *prev;
    struct cJSON *child;
    int           type;
    char         *valuestring;
    int           valueint;
    double        valuedouble;
    char         *string;
} cJSON;

/* 类型常量 */
#define cJSON_Invalid 0
#define cJSON_False   1
#define cJSON_True    2
#define cJSON_NULL    3
#define cJSON_Number  4
#define cJSON_String  5
#define cJSON_Array   6
#define cJSON_Object  7

/* 解析 JSON 字符串, 返回根节点 */
cJSON *cJSON_Parse(const char *value);

/* 从对象中获取子项 */
cJSON *cJSON_GetObjectItem(const cJSON *obj, const char *string);

/* 数组操作 */
int    cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int item);

/* 释放 cJSON 树 */
void   cJSON_Delete(cJSON *c);

#ifdef __cplusplus
}
#endif
#endif
