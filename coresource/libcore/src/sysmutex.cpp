#include "sysmutex.h"

Mutex::Mutex()
{
    Init();
}

Mutex::~Mutex()
{
    UnInit();
}

ST_VOID Mutex::Init()
{
#ifdef _WIN32
	m_Mutex=CreateMutex(NULL, FALSE, NULL);
	m_Cond =CreateEvent(NULL, FALSE, FALSE, NULL );
#else
    pthread_mutex_init(&m_Mutex,NULL);  //函数是以动态方式创建互斥锁的，参数attr指定了新建互斥锁的属性。如果参数attr为NULL，则使用默认的互斥锁属性，默认属性为快速互斥锁 。
    pthread_condattr_init(&m_CondAttr);
    pthread_condattr_setclock(&m_CondAttr, CLOCK_MONOTONIC);
    pthread_cond_init(&m_Cond,&m_CondAttr);
#endif
    m_isLock=false;
}

ST_VOID Mutex::UnInit()
{
#ifdef _WIN32
	CloseHandle(m_Mutex);
	CloseHandle(m_Cond);
#else
    pthread_mutex_destroy(&m_Mutex);
    pthread_condattr_destroy(&m_CondAttr);
    pthread_cond_destroy(&m_Cond);
#endif
}

ST_VOID Mutex::Lock()
{
#ifdef _WIN32
	DWORD d = ::WaitForSingleObject(m_Mutex, INFINITE);
#else
    pthread_mutex_lock(&m_Mutex);
#endif
    m_isLock=true;
}

ST_BOOLEAN Mutex::IsLock()
{
    return m_isLock;
}

ST_VOID Mutex::UnLock()
{
#ifdef _WIN32
	::ReleaseMutex(m_Mutex);
#else
    pthread_mutex_unlock(&m_Mutex);
#endif
    m_isLock=false;
}

ST_VOID Mutex::TryLock()
{
#ifdef _WIN32
#else
    pthread_mutex_trylock(&m_Mutex);
#endif
}

ST_BOOLEAN Mutex::Wait(ST_INT msec)
{
#ifdef _WIN32
	if(msec <= 0) {
		::WaitForSingleObject(m_Cond, INFINITE );
	}
	else {
		int ret = ::WaitForSingleObject(m_Cond, msec);
		if (ret == WAIT_OBJECT_0) {
			return true;
		}
	}
	return false;
#else
    if(msec > 0)
    {
        clock_gettime(CLOCK_MONOTONIC, &m_OutTime);
        long int nsec = m_OutTime.tv_nsec + (msec % 1000) * 1000000L;
        m_OutTime.tv_nsec = (nsec % 1000000000L);
        m_OutTime.tv_sec += (nsec / 1000000000L + msec / 1000);
        return !pthread_cond_timedwait(&m_Cond, &m_Mutex, &m_OutTime);
    }
    else
    {
        return !pthread_cond_wait(&m_Cond, &m_Mutex);
    }
#endif
}

ST_VOID Mutex::Notify()
{
#ifdef _WIN32
	SetEvent(m_Cond);
#else
    pthread_cond_signal(&m_Cond);
#endif
}

ST_VOID Mutex::NotifyAll()
{
#ifdef _WIN32
#else
    pthread_cond_broadcast(&m_Cond);
#endif
}


