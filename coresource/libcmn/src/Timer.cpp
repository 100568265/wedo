//#include "stdafx.h"
#include "Timer.h"

CTimer::CTimer()
{
	m_runing    = false;
	m_func      = NULL;
	m_interval  = -1;
	m_initstate = false;
	m_exit      = false;
}

CTimer::CTimer(const CTimer &timer)
{
	m_runing    = timer.m_runing;
	m_func      = timer.m_func;
	m_interval  = timer.m_interval;
	m_initstate = timer.m_initstate;
	m_exit      = timer.m_exit;
}

ST_VOID CTimer::Init(TIMERROC timerfuc,ST_VOID *param,ST_INT interval)
{
	m_runing=false;
	m_exit=false;
	m_initstate=false;
	m_func=timerfuc;
	m_interval = interval;
	m_param=param;
}

CTimer::~CTimer()
{
	Stop();
}

ST_VOID CTimer::Start()
{
	if(m_exit){
		m_t1 = time(NULL);
		m_exit=false;
		return;
	}
	m_t1 = time(NULL);
	if(m_runing) return;
	m_runing=true;
	m_initstate=false;
	m_t1 = time(NULL);
	m_thread.Start(threadFunc,this,false);
}

ST_VOID CTimer::Start(ST_BOOLEAN initialState)
{
	m_t1 = time(NULL);
	if(m_runing) return;
	m_runing=true;
	m_initstate=initialState;
	m_t1 = time(NULL);
	m_thread.Start(threadFunc,this,false);
}

ST_VOID CTimer::Stop()
{
	if(!m_runing) return;
	m_thread.Stop();
	m_exit=false;
	m_runing=false;
}

ST_VOID CTimer::Stop(ST_BOOLEAN exit)
{
	if(exit){
		Stop();
	}
	else{
		Reset();
		m_exit=!exit;
	}
}

ST_VOID CTimer::Reset()
{
	m_t1 = time(NULL);
	m_t2 = m_t1;
}

#ifdef _WIN32
ST_UINT32 __stdcall CTimer::threadFunc(ST_VOID *pParam)
#else
ST_VOID *CTimer::threadFunc(ST_VOID *pParam)
#endif
{
	double diff = 0;
	CTimer *timer = (CTimer*)pParam;
	if(timer->m_func != NULL && timer->m_initstate && !timer->m_exit) {
		timer->m_func(timer->m_param);
	}
	while(timer->m_runing) {
		if(!timer->m_exit) {
			timer->m_t2 = time(NULL);
			diff = difftime(timer->m_t2, timer->m_t1);
			if((int)diff == timer->m_interval) {
				if(timer->m_func != NULL && !timer->m_exit) {
					timer->m_func(timer->m_param);
				}
				timer->m_t1 = timer->m_t2;
			}
		}
		Thread::SLEEP(1000);
	}
	return 0;
}
