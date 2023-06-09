#ifndef __ATOMIC_H__
#define __ATOMIC_H__

/* x86_64 atomic operations of integers from linux kernel */

#define LOCK_PREFIX "lock; "

/*---------------------------------------------------------------------------*/

/* atomic_t from arch/x86/include/asm/atomic.h */

typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) {i}

static inline void atomic_inc(atomic_t* v)
{
    asm volatile (LOCK_PREFIX "incl %0" : "+m" (v->counter));
}

static inline void atomic_dec(atomic_t* v)
{
    asm volatile (LOCK_PREFIX "decl %0" : "+m" (v->counter));
}

static inline int atomic_read(atomic_t* v)
{
    return v->counter;
}

static inline void atomic_set(atomic_t* v, int i)
{
    v->counter = i;
}

static inline void atomic_add(atomic_t* v, int i)
{
    asm volatile (LOCK_PREFIX "addl %1, %0" : "+m" (v->counter) : "ir" (i));
}

static inline void atomic_sub(atomic_t* v, int i)
{
    asm volatile (LOCK_PREFIX "subl %1, %0" : "+m" (v->counter) : "ir" (i));
}

static inline int xadd(atomic_t* v, int i)
{
    asm volatile (LOCK_PREFIX "xaddl %0, %1"
                  : "+r" (i), "+m" (v->counter)
                  : : "memory", "cc");
    return i;
}

static inline long atomic_add_return(atomic_t* v, int i)
{
    return i + xadd(v, i);
}

static inline long atomic_sub_return(atomic_t* v, int i)
{
    return atomic_add_return(v, -i);
}

#define atomic_inc_return(v) atomic_add_return((v), 1)
#define atomic_dec_return(v) atomic_sub_return((v), 1)

/*---------------------------------------------------------------------------*/

/* atomic64_t on x64 from arch/x86/include/asm/atomic64_64.h */

typedef struct {
    volatile long counter;
} atomic64_t;

#define ATOMIC64_INIT(i) {i}

static inline void atomic64_inc(atomic64_t* v)
{
    asm volatile (LOCK_PREFIX "incq %0" : "+m" (v->counter));
}

static inline void atomic64_dec(atomic64_t* v)
{
    asm volatile (LOCK_PREFIX "decq %0" : "+m" (v->counter));
}

static inline long atomic64_read(atomic64_t* v)
{
    return v->counter;
}

static inline void atomic64_set(atomic64_t* v, long i)
{
    v->counter = i;
}

static inline void atomic64_add(atomic64_t* v, long i)
{
    asm volatile (LOCK_PREFIX "addq %1, %0" : "+m" (v->counter) : "er" (i));
}

static inline void atomic64_sub(atomic64_t* v, long i)
{
    asm volatile (LOCK_PREFIX "subq %1, %0" : "+m" (v->counter) : "er" (i));
}

static inline long xadd64(atomic64_t* v, long i)
{
    asm volatile (LOCK_PREFIX "xaddq %0, %1"
                  : "+r" (i), "+m" (v->counter)
                  : : "memory", "cc");
    return i;
}

static inline long atomic64_add_return(atomic64_t* v, long i)
{
    return i + xadd64(v, i);
}

static inline long atomic64_sub_return(atomic64_t* v, long i)
{
    return atomic64_add_return(v, -i);
}

#define atomic64_inc_return(v) atomic64_add_return((v), 1)
#define atomic64_dec_return(v) atomic64_sub_return((v), 1)

#endif
