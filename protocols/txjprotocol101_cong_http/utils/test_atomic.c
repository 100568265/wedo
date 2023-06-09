#include <stdio.h>
#include <pthread.h>
#include "atomic.h"

#ifdef USE_ASM

static atomic64_t counter;

#else

static long counter;

#ifdef USE_MUTEX
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#elif defined USE_SPINLOCK
static pthread_spinlock_t spinlock;
#endif

#endif

void* func(void* nil)
{
    int i;

    for (i = 0; i < 10000; ++i) {
#ifdef USE_ASM
        atomic64_inc(&counter);
#elif defined USE_MUTEX
        pthread_mutex_lock(&mutex);
        ++counter;
        pthread_mutex_unlock(&mutex);
#elif defined USE_SPINLOCK
        pthread_spin_lock(&spinlock);
        ++counter;
        pthread_spin_unlock(&spinlock);
#elif defined USE_BUILTIN
        __sync_fetch_and_add(&counter, 1);
#else
        ++counter;
#endif
    }

    return NULL;
}

#define NR_THREAD 1000

static inline void test_inc(void)
{
    int i;
    pthread_t pid[NR_THREAD];

#ifdef USE_SPINLOCK
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);
#endif

    for (i = 0; i < NR_THREAD; ++i)
        pthread_create(&pid[i], NULL, func, NULL);

    for (i = 0; i < NR_THREAD; ++i)
        pthread_join(pid[i], NULL);

#ifdef USE_ASM
    printf("result: %ld\n", atomic64_read(&counter));
#else
    printf("result: %ld\n", counter);
#endif
}

static inline void test_64_return(void)
{
    long old;
    atomic64_t v = ATOMIC64_INIT(5);

    printf("orig value = %ld\n", atomic64_read(&v));

    old = xadd64(&v, 15);
    printf("old value = %ld, new value = %ld\n", old, atomic64_read(&v));

    old = xadd64(&v, -3);
    printf("old value = %ld, new value = %ld\n", old, atomic64_read(&v));
}

static inline void test_return(void)
{
    int old;
    atomic_t v = ATOMIC_INIT(5);

    printf("orig value = %d\n", atomic_read(&v));

    old = xadd(&v, 15);
    printf("old value = %d, new value = %d\n", old, atomic_read(&v));

    old = xadd(&v, -3);
    printf("old value = %d, new value = %d\n", old, atomic_read(&v));
}

int main(void)
{
    test_inc();
    printf("----------------------------\n");
    test_return();
    printf("----------------------------\n");
    test_64_return();

    return 0;
}
