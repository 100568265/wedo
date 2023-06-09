
#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#include <string>

class WatchDogBase
{
public:
	///< 错误标志集合
/*	enum Errno {
		NOT_ERROR = 0,			///< 无错误标志
		NOT_SETTIMEOUT,			///< 不能设置超时时间标志
		NOT_GETTTIMEOUT,		///< 不能获取当前设置的超时时间标志
		NOT_SETPRETTIMEOUT,		///< 不能设置预超时值标志
		NOT_GETPRETTIMEOUT,		///< 不能获取预超时值标志
		NOT_GET_TIMELEFT,		///< 不能获取剩余超时时间标志
		NOT_GETSUPPORT, 		///< 不能获取关于系统的信息标志
		NOT_FIND_WATCHDOG,      ///< 找不到看门狗标志
		FEED_DOG_FAIL,          ///< 喂狗失败标志
		NOT_DISABLECARD,		///< 不能暂停加密狗定时器标志
		NOT_ENABLECARD			///< 不能使能加密狗定时器标志
	};
*/
	WatchDogBase():
		/*_errno(NOT_ERROR), */iv_sec(0) {}
	WatchDogBase(int sec):
		/*_errno(NOT_ERROR), */iv_sec(sec)
		{ if (sec < 60) iv_sec = 60; }
	virtual ~WatchDogBase() {}

	virtual int Init () { return 0; };
	virtual int Start() = 0;
	virtual int Feeding () = 0;
	virtual int Start(int timeout) { return 0; };
	virtual int SetTimeout(int timeout) { return 0; };
	virtual int Stop () { return 0; };

	// inline Errno err_no () const
	// 	{ return (Errno)_errno; }
	// std::string strerror () const;
protected:
	int _errno;
	int iv_sec;
};

class LannerWatchDog : public WatchDogBase
{
public:
	LannerWatchDog();
	LannerWatchDog(int sec);
	virtual ~LannerWatchDog();
	
	int Init ();
	int Start();
	int Feeding ();
	int SetTimeout (int timeout);
	int Stop ();

private:
	int _devid;
	int _value;

	std::string _path;
};

class LightWatchDog : public WatchDogBase
{
public:	
	LightWatchDog();
	LightWatchDog(int timeout);
	virtual ~LightWatchDog();
	
	int Start();
	int Feeding ();
	int SetTimeout(int timeout);

private:
	friend class WatchDogPrivate;
	class WatchDogPrivate* _pri;
};

#endif // _WATCHDOG_H_