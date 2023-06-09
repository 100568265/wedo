#include <ctype.h>

static int hex_value[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0 - 15 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16 - 31 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 32 - 47 */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 48 - 63 */
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 64 - 79 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 - 95 */
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 96 - 111 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 112 - 127 */
};

int ndec2int(const char* src, unsigned int len, int* dst) {
    unsigned int i;
    int result = 0;

    for (i = 0; i < len; ++i) {
        if (!isdigit(src[i])) {
            return -1;
        }

        result = result * 10 + (src[i] - '0');
    }

    *dst = result;
    return 0;
}

int ndec2long(const char* src, unsigned int len, long* dst) {
    unsigned int i;
    int result = 0;

    for (i = 0; i < len; ++i) {
        if (!isdigit(src[i])) {
            return -1;
        }

        result = result * 10 + (src[i] - '0');
    }

    *dst = result;
    return 0;
}

int hex2int(const char* src, int* dst) {
    unsigned int i;
    int result = 0;

    for (i = 0; src[i]; ++i) {
        if (!isxdigit(src[i])) {
            return -1;
        }

        result = (result << 4) + hex_value[(int)(src[i])];
    }

    *dst = result;
    return 0;
}

int nhex2int(const char* src, unsigned int len, int* dst) {
    unsigned int i;
    int result = 0;

    for (i = 0; i < len; ++i) {
        if (!isxdigit(src[i])) {
            return -1;
        }

        result = (result << 4) + hex_value[(int)(src[i])];
    }

    *dst = result;
    return 0;
}

int hex2long(const char* src, long* dst) {
    unsigned int i;
    long result = 0;

    for (i = 0; src[i]; ++i) {
        if (!isxdigit(src[i])) {
            return -1;
        }

        result = (result << 4) + hex_value[(int)(src[i])];
    }

    *dst = result;
    return 0;
}

int nhex2long(const char* src, unsigned int len, long* dst) {
    unsigned int i;
    long result = 0;

    for (i = 0; i < len; ++i) {
        if (!isxdigit(src[i])) {
            return -1;
        }

        result = (result << 4) + hex_value[(int)(src[i])];
    }

    *dst = result;
    return 0;
}

int bin2int(const char* src, int* dst) {
    unsigned int i;
    int result = 0;

    for (i = 0; src[i]; ++i) {
        if (src[i] != '0' && src[i] != '1') {
            return -1;
        }

        result = (result << 1) + (src[i] - '0');
    }

    *dst = result;
    return 0;
}

int bin2long(const char* src, long* dst) {
    unsigned int i;
    long result = 0;

    for (i = 0; src[i]; ++i) {
        if (src[i] != '0' && src[i] != '1') {
            return -1;
        }

        result = (result << 1) + (src[i] - '0');
    }

    *dst = result;
    return 0;
}
