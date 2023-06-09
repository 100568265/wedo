#include <stdio.h>
#include <string.h>
#include "hash.h"

static struct {
    const char* name;
    unsigned long (*func)(const char*, unsigned int);
} hash_func_table[] = {
    {"simple_hash11",   simple_hash11},
    {"simple_hash31",   simple_hash31},
    {"djb_hash",        djb_hash},
    {"sdbm_hash",       sdbm_hash},
    {"dek_hash",        dek_hash},
    {"bp_hash",         bp_hash},
    {"pjw_hash",        pjw_hash},
    {NULL,              NULL},
};

static inline void test_hash_func(const char* buf, int len)
{
    int i;

    for (i = 0; hash_func_table[i].func; ++i)
        printf("%s(\"%s\")\t=\t%lu\n", hash_func_table[i].name, buf,
               hash_func_table[i].func(buf, len));
}

int main(void)
{
    const char* str = "Hello, world!";

    test_hash_func(str, strlen(str));

    return 0;
}
