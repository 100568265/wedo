#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef NULL
#define NULL 0
#endif

#define IS2EXP(n)    (((n) > 0) ? (!((n) & ((n) - 1))) : 0)

#define ALIGN(n, align) (((n) + (align) - 1) & (~((align) - 1)))

#define UPPER(size, n)  (((size) + ((n) - 1)) / (n))

#define offset_of(type, member) \
    ((unsigned long)(&(((type*)0)->member)))

#define container_of(ptr, type, member) \
    ((type*)((unsigned long)(ptr) - offset_of(type, member)))

#endif
