#include <stdio.h>
#include <time.h>

/* implementation of mktime() from linux kernel */

/**
 * mktime() implementation from kernel/time.c.
 * Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 **/
static inline unsigned long long
__mktime(const unsigned int year0, const unsigned int mon0,
         const unsigned int day, const unsigned int hour,
         const unsigned int min, const unsigned int sec)
{
    unsigned int mon = mon0, year = year0;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int) (mon -= 2)) {
        mon += 12;      /* Puts Feb last since it has leap day */
        year -= 1;
    }   

    return ((((unsigned long long)
              (year/4 - year/100 + year/400 + 367*mon/12 + day) +
              year*365 - 719499
             )*24 + hour /* now have hours */
            )*60 + min /* now have minutes */
           )*60 + sec; /* finally seconds */
}

unsigned long long str2gmtime(const char* time_str)
{
    int year, mon, day, hour, min, sec;

    sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &mon, &day,
           &hour, &min, &sec);

    return __mktime(year, mon, day, hour, min, sec);
}

void gmtime2str(unsigned long long timeval, void* buf, int buflen)
{
    struct tm* tp;

    tp = gmtime((const time_t*)(&timeval));
    strftime(buf, buflen, "%F %T", tp);
}

void get_local_time(char* time_str, int len)
{
    time_t t;
    struct tm* tp;

    t = time(NULL);
    tp = localtime(&t);
    strftime(time_str, len, "%F %T", tp);
}
