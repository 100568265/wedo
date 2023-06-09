
#ifndef _DATE_TIME_H_
#define _DATE_TIME_H_

#include <time.h>

class DateTime
{
public:
	DateTime();
    ~DateTime();

	static bool   utctime   (time_t now, struct tm& save);
	static bool   localtime (time_t now, struct tm& save);
};

#endif // _DATE_TIME_H_
