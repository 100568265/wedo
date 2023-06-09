
#ifndef _WATCHDOG_HELPER_H_
#define _WATCHDOG_HELPER_H_


class WatchdogHelper
{
public:
	WatchdogHelper();
	virtual ~WatchdogHelper();

	int Init ();
	int  Run ();
private:
	class WatchdogHelperPrivate* _pri;
};

#endif //_WATCHDOG_HELPER_H_