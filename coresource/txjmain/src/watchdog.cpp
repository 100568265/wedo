
#include "watchdog.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if defined(__linux__)
	#include <sys/io.h>
	#include <linux/watchdog.h>
#endif

//#include "ioaccess.h"
//#include "wd_ioctl.h"

/*std::string WatchDogBase::strerror () const
{
	static const char* errstr[] = {
		"Not Error.",
		"The watchdog does not support setting timeout.",
		"The watchdog does not support the acquisition timeout.",
		"The watchdog does not support setting Pretimeouts.",
		"The watchdog does not support the acquisition Pretimeouts.",
		"The watchdog does not support the acquisition of the remaining timeout.",
		"The watchdog does not support access to system information.",
		"Can not find watchdogs, Please make sure you let it obediently at home gatekeeper.",
		"To feed the dog fail.",
		"Cannot pause watchdog, It is very excited.",
		"Cannot enable watchdog, It doesn't seem to want to help you with your work."
	};
	return errstr[_errno];
}*/

//===============================================================================//

LannerWatchDog::LannerWatchDog():
	_devid(-1),
	_value(-1),
	_path ("/dev/wd_drv")
{
}

LannerWatchDog::LannerWatchDog(int sec):
	WatchDogBase(sec),
	_devid(-1),
	_value(-1),
	_path ("/dev/wd_drv")
{
	if (sec > 255) iv_sec = 255;
}

LannerWatchDog::~LannerWatchDog()
{
}

int LannerWatchDog::Init ()
{
#if ( defined(DIRECT_IO_ACCESS) && !defined(DJGPP) )
	#if defined(__linux__)
	iopl(3);
	#endif
	#if defined(__FreeBSD__)
    int iofl;
    SET_IOPL();
	#endif
#endif

#if defined(DIRECT_IO_ACCESS)
	wd_gpio_init();
	return 0;
#else
    _devid = open(_path.c_str(), O_RDONLY);

    return (_devid < 0 ? -1 : 0);
#endif
}

int LannerWatchDog::Start()
{
#if !defined(DIRECT_IO_ACCESS)
//	_value = START_WDT;
//	return ioctl(_devid, IOCTL_START_STOP_WDT, &_value);
#endif
	return 0;
}

int LannerWatchDog::Feeding ()
{
#if !defined(DIRECT_IO_ACCESS)
	return Start();
#else
	return 0;
#endif
}

int LannerWatchDog::SetTimeout(int timeout)
{
	if (timeout < 1) return -1;
	if (iv_sec < 60 ) iv_sec = 60;
	if (iv_sec > 255) iv_sec = 255;

#if defined(DIRECT_IO_ACCESS)
   	start_watchdog_timer(iv_sec);
   	return 0;
#else
//	return ioctl(_devid, IOCTL_SET_WDTO_TIMER, &iv_sec);
#endif
}

int LannerWatchDog::Stop ()
{
#if defined(DIRECT_IO_ACCESS)
	stop_watchdog_timer();
	return 0;
#else
//	_value = STOP_WDT;
//	return ioctl(_devid, IOCTL_START_STOP_WDT, &_value);
#endif
}

//===============================================================================//

class WatchDogPrivate
{
public:
	WatchDogPrivate ():
		wd_fd(-1), path("/dev/watchdog") {}

		// 获得狗
	inline void getdog  () {
		if (wd_fd < 0)
			this->wd_fd = open(path.c_str(), O_RDWR);
	}
		// 放养狗
	inline void stocking() {
		if (wd_fd >= 0) close(this->wd_fd);
		this->wd_fd = -1;
	}
		// 让狗看门
	inline int wacth () const {
		if (wd_fd < 0) return -1;
		int options = WDIOS_ENABLECARD;
		return ioctl(wd_fd, WDIOC_SETOPTIONS, &options);
	}
		// 让狗待命
	inline int standby () const {
		if (wd_fd < 0) return -1;
		int options = WDIOS_DISABLECARD;
		return ioctl(wd_fd, WDIOC_SETOPTIONS, &options);
	}

		// 喂养狗
	inline int feeddog () const {
		if (wd_fd < 0) return -1;
		return ioctl(wd_fd, WDIOC_KEEPALIVE, NULL);
	}
		// 设置饿死超时
	inline int setTimeout (int to) const {
		if (wd_fd < 0) return -1;
		if (ioctl(wd_fd, WDIOC_SETTIMEOUT, &to) < 0)
			return -1;
		return to;
	}

	int wd_fd;
	std::string path;
};

LightWatchDog::LightWatchDog():
	_pri(new WatchDogPrivate())
{
}

LightWatchDog::LightWatchDog(int timeout):
	WatchDogBase(timeout),
	_pri(new WatchDogPrivate())
{
}

LightWatchDog::~LightWatchDog()
{
	_pri->stocking();
	delete _pri;
}

int LightWatchDog::Start()
{
	_pri->getdog ();
	if (_pri->wd_fd < 0) {
		// _errno = WatchDogBase::NOT_FIND_WATCHDOG;
		return -1;
	}
	SetTimeout (iv_sec);
	return 0;
}

int LightWatchDog::Feeding ()
{
	int ret = _pri->feeddog ();
	if (ret < 0) {
		// _errno = WatchDogBase::FEED_DOG_FAIL;
		return -1;
	}
	return 0;
}

int LightWatchDog::SetTimeout(int timeout)
{
	if (timeout < 1) return -1;
	if (iv_sec < 60 ) iv_sec = 60;
	return _pri->setTimeout(iv_sec);
}
