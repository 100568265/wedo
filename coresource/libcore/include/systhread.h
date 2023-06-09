#ifndef SYSTHREAD_H
#define SYSTHREAD_H

#include "datatype.h"

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
#endif

#ifdef _WIN32
    typedef ST_UINT32 (__stdcall * THREADPROC)(ST_VOID * param);
#else
    typedef ST_VOID * (*THREADPROC)(ST_VOID * param);
#endif

class Thread
{
public:

    Thread();
    ~Thread();
    ST_INT						Start(THREADPROC proc,ST_VOID *param,ST_BOOLEAN loop);
    ST_VOID 					Stop ();
	ST_VOID 					Stop (ST_INT msec);
    static ST_VOID              SLEEP(ST_INT msec);

private:
#ifdef _WIN32
    static ST_UINT32 __stdcall  ThreadProc(ST_VOID *param);
#else
    static ST_VOID             *ThreadProc(ST_VOID *param);
#endif

#ifdef _WIN32
	HANDLE						m_Thread_Event;
	HANDLE						m_Thread_Id;
#else
	pthread_t					m_Thread_Id;
#endif
    ST_BOOLEAN					m_IsRuning;
    ST_BOOLEAN					m_IsLoop;
    ST_VOID					   *m_pParam;
    THREADPROC					m_pProc;
};

#endif // SYSTHREAD_H
