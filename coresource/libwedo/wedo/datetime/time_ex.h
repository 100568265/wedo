
#ifndef _WEDO_DATETIME_TIME_EX_H_
#define _WEDO_DATETIME_TIME_EX_H_

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
#    pragma once
#endif

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
struct timespec {
	time_t tv_sec;
	long  tv_nsec;
};

enum {
    CLOCK_REALTIME,
    CLOCK_REALTIME_COARSE
};

int  clock_gettime(int clk_id, struct timespec * tp);
#endif

void localtime_mt (time_t , struct tm *);
void gmtime_mt    (time_t , struct tm *);

void asctime_mt   (const struct tm *, char * buf, size_t buflen);
void ctime_mt     (time_t , char * buf, size_t buflen);

#ifdef __cplusplus
} // extern "C"
#endif

namespace wedo {

inline struct tm localtime (time_t t)
{
    struct tm st_tm;
    localtime_mt (t, &st_tm);
    return st_tm;
}

inline struct tm gmtime    (time_t t)
{
    struct tm st_tm;
    gmtime_mt    (t, &st_tm);
    return st_tm;
}

};

#endif // _WEDO_DATETIME_TIME_EX_H_
