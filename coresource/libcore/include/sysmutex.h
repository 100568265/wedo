#ifndef METUX_H
#define METUX_H


#include "datatype.h"

#if   defined( _WIN32  )
    #include <windows.h>
#elif defined(__linux__)
    #include <sys/time.h>
    #include <pthread.h>
#endif


class Mutex
{
public:
    Mutex();
    virtual ~Mutex();
    ST_VOID 			Lock();
    ST_VOID 			UnLock();
    ST_VOID 			TryLock();
    ST_BOOLEAN 			Wait(ST_INT mSec);
    ST_VOID 			Notify();
    ST_VOID 			NotifyAll();
    ST_BOOLEAN          IsLock();
protected:
    ST_VOID 			Init();
    ST_VOID 			UnInit();
protected:
#ifdef _WIN32
	HANDLE 				m_Mutex;
	HANDLE 				m_Cond;
#else
    struct timespec 	m_OutTime;
    pthread_mutex_t 	m_Mutex;
    pthread_cond_t		m_Cond;
    pthread_condattr_t	m_CondAttr;
#endif
	ST_BOOLEAN          m_isLock;

};

class Locker {
public:
    Locker(Mutex* mutex = 0) : _pmutex(mutex), _islock(false)
    {
        if (_pmutex) { _pmutex->Lock(); _islock = true; }
    }
    Locker(const Locker& locker) : _pmutex(0), _islock(false) {}
    ~Locker() { if (_pmutex) _pmutex->UnLock(); }

    void Lock (Mutex* mutex = 0) {
        if (_islock) return;
        if (mutex) { _pmutex = mutex; _pmutex->Lock(); _islock = true; }
    }

    Locker& operator= (const Locker& locker) { return *this; }
private:
    Mutex* _pmutex;
    bool   _islock;
};


#endif // METUX_H
