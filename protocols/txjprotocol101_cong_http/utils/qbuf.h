#ifndef __QBUF_H__
#define __QBUF_H__

#include <stdlib.h>
#include <string.h>

struct qbuf { /* DONOT visit fileds directly */
    char* __base__;
    unsigned int __size__;
    unsigned int __capacity__;
};

struct qbuf_ref {
    const char* base;
    unsigned int size;
};

static inline void qbuf_ref_init(struct qbuf_ref* r) {
    r->base = NULL;
    r->size = 0;
}

static inline void qbuf_ref_destroy(struct qbuf_ref* r) {
    r->base = NULL;
    r->size = 0;
}

static inline void qbuf_init(struct qbuf* q) {
    q->__base__ = NULL;
    q->__size__ = 0;
    q->__capacity__ = 0;
}

static inline void qbuf_destroy(struct qbuf* q) {
    if (q->__base__) {
        free(q->__base__);
        qbuf_init(q);
    }
}

static inline unsigned int qbuf_size(const struct qbuf* q) {
    return q->__size__;
}

static inline void* qbuf_data(const struct qbuf* q) {
    return q->__base__;
}

static inline int qbuf_empty(const struct qbuf* q) {
    return (q->__size__ == 0);
}

static inline void qbuf_clear(struct qbuf* q) {
    q->__size__ = 0;
}

static inline int qbuf_reserve(struct qbuf* q, unsigned int expected_size) {
    if (expected_size > q->__capacity__) {
        unsigned int new_capacity;
        if (q->__capacity__ == 0) {
            new_capacity = 16;
        } else {
            new_capacity = q->__capacity__;
        }

        do {
            new_capacity <<= 1;
        } while (new_capacity < expected_size);

        char* new_base = (char*)realloc(q->__base__, new_capacity);
        if (!new_base) {
            return -1;
        }

        q->__base__ = new_base;
        q->__capacity__ = new_capacity;
    }

    return 0;
}

static inline int qbuf_resize(struct qbuf* q, unsigned int expected_size) {
    int err = qbuf_reserve(q, expected_size);
    if (err) {
        return err;
    }

    q->__size__ = expected_size;
    return 0;
}

static inline int qbuf_append(struct qbuf* q, const void* data, unsigned int size) {
    if (qbuf_reserve(q, q->__size__ + size) != 0) {
        return -1;
    }

    memcpy(q->__base__ + q->__size__, data, size);
    q->__size__ += size;
    return 0;
}

static inline int qbuf_assign(struct qbuf* q, const void* data, unsigned int size) {
    qbuf_clear(q);
    return qbuf_append(q, data, size);
}

static inline void qbuf_swap(struct qbuf* a, struct qbuf* b) {
    struct qbuf tmp = *a;
    *a = *b;
    *b = tmp;
}

#endif
