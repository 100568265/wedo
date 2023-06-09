#ifndef CTIME_H
#define CTIME_H

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#endif
#include "datatype.h"
#include "systhread.h"
#include "GlobalCfg.h"

#ifdef _WIN32
typedef ST_UINT32 (__stdcall *TIMERROC)(ST_VOID *param);
#else
typedef ST_VOID  *(*TIMERROC)(ST_VOID *param);
#endif

#ifdef _WIN32
ST_VOID  __stdcall timerfunc(ST_VOID *param);
#else
ST_VOID  timerfunc(ST_VOID *param);
#endif

#ifdef _WIN32
class __declspec(dllexport) Thread;
class COMMUNICATIONDLL_API CTimer
#else
class CTimer
#endif
{
public:
	CTimer();
	CTimer(const CTimer &timer);
	~CTimer();
	ST_VOID Init(TIMERROC timerfuc,ST_VOID *param,ST_INT interval);
	ST_VOID Reset();
	ST_VOID Start();
	ST_VOID Start(ST_BOOLEAN initialState);
	ST_VOID Stop();
	ST_VOID Stop(ST_BOOLEAN exit);
private:
	#ifdef _WIN32
		static ST_UINT32 __stdcall threadFunc (ST_VOID *pParam);
	#else
		static ST_VOID *threadFunc (ST_VOID  *pParam);
	#endif
private:
	TIMERROC m_func;
	Thread m_thread;
	time_t m_t1, m_t2;
	ST_BOOLEAN m_runing;
	ST_BOOLEAN m_exit;
	ST_INT m_interval;
	ST_BOOLEAN m_initstate;
	ST_VOID *m_param;
};

#endif
