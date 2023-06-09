unsigned long simple_hash11(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = 0;

    for (i = 0; i < len; ++i)
        hash += (hash << 3) + (hash << 1) + str[i];

    return hash;
}

unsigned long simple_hash31(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = 0;

    for (i = 0; i < len; ++i)
        hash = (hash << 5) - hash + str[i];

    return hash;
}

unsigned long djb_hash(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = 5381;

    for (i = 0; i < len; ++i)
        hash += (hash << 5) + str[i];

    return hash;
}

unsigned long sdbm_hash(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = 0;

    for (i = 0; i < len; ++i)
        hash = str[i] + (hash << 6) + (hash << 16) - hash;

    return hash;
}

unsigned long dek_hash(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = len;

    for (i = 0; i < len; ++i)
        hash = (hash << 5) ^ (hash >> 27) ^ str[i];

    return hash;
}

unsigned long bp_hash(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long hash = 0;

    for (i = 0; i < len; ++i)
        hash = hash << 7 ^ str[i];

    return hash;
}

unsigned long pjw_hash(const char* str, unsigned int len) {
    register unsigned int i;
    register unsigned long g, hash = 0;

    for (i = 0; i < len; ++i) {
        hash = (hash << 4) + str[i];
        g = hash & 0xf0000000;
        if (g) {
            hash ^= (g >> 24);
            hash ^= g;
        }
    }

    return hash;
}
