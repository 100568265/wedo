#include "cJSON.h"

extern cJSON* cJSON_Parse(const char* value);

extern void cJSON_Delete(cJSON *c);
//这个生成的字符串有做格式调整
extern char *cJSON_Print(cJSON *item);
//这个没做格式调整，就是一行字符串显示
extern char *cJSON_PrintUnformatted(cJSON *item);
