#ifndef __SEQLOCK_H__
#define __SEQLOCK_H__

#include <pthread.h>

/* a simple userspace sequence lock implementation */

struct seqlock {
    unsigned int seq;
    pthread_spinlock_t lock;
};

static inline void seqlock_init(struct seqlock* sl)
{
    sl->seq = 0;
    pthread_spin_init(&sl->lock, PTHREAD_PROCESS_PRIVATE);
}

static inline void seqlock_wr_lock(struct seqlock* sl)
{
    pthread_spin_lock(&sl->lock);
    ++sl->seq;
}

static inline void seqlock_wr_unlock(struct seqlock* sl)
{
    ++sl->seq;
    pthread_spin_unlock(&sl->lock);
}

static inline unsigned int seqlock_rd_begin(struct seqlock* sl)
{
    register unsigned int ret;

again:
    ret = sl->seq;
    if (ret & 1)
        goto again;

    return ret;
}

static inline int seqlock_rd_retry(struct seqlock* sl, unsigned int start)
{
    return (sl->seq != start);
}

#endif
