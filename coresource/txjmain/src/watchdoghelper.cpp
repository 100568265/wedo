
#include "watchdoghelper.h"

#include "watchdog.h"
#include "systhread.h"
#include <stdio.h>
#include <unistd.h>

class WatchdogHelperPrivate
{
	friend class WatchdogHelper;
public:
	WatchdogHelperPrivate(): _timeout(120), _feed_iv(10),
		_wdptr(new LightWatchDog(_timeout)), _isInit(false) {}
	~WatchdogHelperPrivate() { delete _wdptr; }

	static void* func (void * param) {
		WatchdogHelperPrivate* helper = (WatchdogHelperPrivate*)param;
		sleep (helper->_feed_iv);
		helper->_wdptr->Feeding();
		return 0;
	}

private:
	long          _timeout;
	long          _feed_iv;
	Thread        _thread;
	WatchDogBase* _wdptr;
	bool          _isInit;
};

WatchdogHelper::WatchdogHelper():
	_pri(new WatchdogHelperPrivate())
{
}

WatchdogHelper::~WatchdogHelper()
{
	_pri->_wdptr->Stop ();
	_pri->_thread.Stop ();
	delete _pri;
}

int WatchdogHelper::Init()
{
	FILE* fp = fopen ("./config/watchdog.conf", "r");
	if (fp) {
		fclose (fp);
		return 0;
	}
	if (_pri->_wdptr->Init () < 0) {
		return -1;
	}
	_pri->_isInit = true;
	return 0;
}

int WatchdogHelper::Run ()
{
	if (!_pri->_isInit) return -1;
	if (_pri->_thread.Start (_pri->func, _pri, true) < 0)
		return -1;
	return _pri->_wdptr->Start();
}
