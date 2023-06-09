
#include "wedo/datetime/time_ex.h"

#ifdef _WIN32

inline LARGE_INTEGER
get_offset_by_clock_gettime()
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

int
clock_gettime(int clk_id, struct timespec *tp)
{
	LARGE_INTEGER           t;
	FILETIME				f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
        offset = get_offset_by_clock_gettime();
        frequencyToMicroseconds = 10.;
	}

    GetSystemTimeAsFileTime(&f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart  = microseconds;
	tp->tv_sec  = t.QuadPart / 1000000;
	tp->tv_nsec = (t.QuadPart % 1000000) * 1000;
	return (0);
}
#endif // _WIN32

void localtime_mt (time_t t, struct tm * tmptr)
{
    if(t) {
#ifdef _WIN32
	localtime_s(tmptr, &t);
#else
	localtime_r(&t, tmptr);
#endif
    }
    else {
    time_t now = time(0);
#ifdef _WIN32
	localtime_s(tmptr, &now);
#else
	localtime_r(&now, tmptr);
#endif
    }
}

void gmtime_mt    (time_t t, struct tm * tmptr)
{
    if(t) {
#ifdef _WIN32
    gmtime_s(tmptr, &t);
#else
    gmtime_r(&t, tmptr);
#endif
    }
    else {
    time_t now = time(0);
#ifdef _WIN32
	gmtime_s(tmptr, &now);
#else
	gmtime_r(&now, tmptr);
#endif
    }
}

void asctime_mt(const struct tm * tmptr, char * buf, size_t buflen)
{
    if (tmptr) {
#ifdef _WIN32
    asctime_s (buf, buflen, tmptr);
#else
    asctime_r (tmptr, buf);
#endif
    }
    else {
    struct tm now;
    gmtime_mt (0, &now);
#ifdef _WIN32
    asctime_s (buf, buflen, &now);
#else
    asctime_r (&now, buf);
#endif
    }
}

void ctime_mt  (time_t t, char * buf, size_t buflen)
{
    if (t) {
#ifdef _WIN32
    ctime_s (buf, buflen, &t);
#else
    ctime_r (&t, buf);
#endif
    }
    else {
    time_t now = time(0);
#ifdef _WIN32
    ctime_s (buf, buflen, &now);
#else
    ctime_r (&now, buf);
#endif
    }
}

namespace wedo {

};
