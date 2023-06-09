#include <stdio.h>
#include "str_utils.c"

int main(void)
{
    int i;
    const char* str[] = {"cc", "e8", "cf", "7f", NULL};

    for (i = 0; str[i]; ++i) {
        int result;
        if (hex2int(str[i], &result) == 0)
            printf("%s -> %d\n", str[i], result);
        else
            printf("convert error.\n");
    }

    return 0;
}

