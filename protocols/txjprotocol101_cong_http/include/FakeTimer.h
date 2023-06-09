
#ifndef _FAKETIMER_H
#define _FAKETIMER_H

#include <time.h>

class FakeTimer {

public:
	FakeTimer (int itnl = 0) : begin_sec(0),
		end_sec(0), t_internal(itnl), bStart(0) {}
	~FakeTimer () {}

	time_t internal () const { return t_internal; }
	time_t remainingTime () const {
		return t_internal - ( bStart ? curr_sec : end_sec) + begin_sec;
	}
	void setInternal (time_t sec) { t_internal = sec; }

	static void timing () { curr_sec = time(0); }
	bool isTimeout () const {
		return t_internal < (bStart ? (curr_sec - begin_sec) : false);
	}

	void start (time_t sec) { t_internal = sec; start(); }
	void start () { bStart = true; curr_sec = begin_sec = time(0); }
	void stop () { bStart = false; end_sec  = time(0); }

private:
    static volatile time_t curr_sec;
    FakeTimer (FakeTimer& ft) {}

	volatile time_t begin_sec;
	volatile time_t end_sec;
	volatile time_t t_internal;
	volatile bool   bStart;
};

#endif // !_FAKETIMER_H
