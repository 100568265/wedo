#include "Thread.h"

#include <errno.h>
#include <time.h>

Thread::Thread()
{
    m_IsLoop=false;
    m_IsRuning=false;
#ifdef _WIN32
	m_Thread_Id=NULL;
	m_Thread_Event=NULL;
#endif
}

Thread::~Thread()
{
#ifdef _WIN32
	if (m_Thread_Id!=NULL)
		CloseHandle(m_Thread_Id);
	if(m_Thread_Event!=NULL)
		CloseHandle(m_Thread_Event);
#endif
}

#ifdef _WIN32
ST_UINT32 __stdcall  Thread::ThreadProc(ST_VOID *param)
#else
ST_VOID *            Thread::ThreadProc(ST_VOID *param)
#endif
{
    Thread *pThread=(Thread*)param;
    if(pThread->m_pProc!=NULL)
    {
        if(!pThread->m_IsLoop){
            pThread->m_pProc(pThread->m_pParam);
        }
        else{
            //实现线程无限循环
            while(pThread->m_IsLoop && pThread->m_IsRuning){
                pThread->m_pProc(pThread->m_pParam);
    #ifdef _WIN32
                if(!pThread->m_IsRuning){
                    ::SetEvent(pThread->m_Thread_Event);
                    return 0;
                }
    #endif
            }
    #ifdef _WIN32
            if(!pThread->m_IsRuning){
                ::SetEvent(pThread->m_Thread_Event);
            }
    #endif
        }
    }
    return 0;
}

ST_INT Thread::Start(THREADPROC proc,ST_VOID *param,ST_BOOLEAN loop)
{
    ST_INT err;
    m_pProc=proc;
    m_pParam=param;
    m_IsLoop=loop;
#ifdef _WIN32
	if(m_Thread_Event == NULL){
		m_Thread_Event = CreateEvent(NULL,SD_TRUE,SD_FALSE,NULL);
	}
	if(m_Thread_Id==NULL){
		m_Thread_Id =CreateThread(NULL,0,ThreadProc,this,0,NULL);
	}
	err=GetLastError();
#else
    err=pthread_create(&m_Thread_Id,NULL,ThreadProc,this);
#endif
	if(0!=err){
        m_IsRuning=false;
    }
    else{
        m_IsRuning=true;
    }
	return err;
}

ST_VOID Thread::Stop()
{
    m_IsRuning=false;
#ifdef _WIN32
	::WaitForSingleObject(m_Thread_Event,INFINITE);
	//::WaitForSingleObject(m_Thread_Event,1000);
	//::TerminateThread(m_Thread_Id,0);
#else
     pthread_join(m_Thread_Id,NULL); //pthread_join用于等待一个线程的结束，也就是主线程中要是加了这段代码，就会在加代码的位置卡主，直到这个线程执行完毕才往下走。
#endif
}

ST_VOID Thread::Stop(ST_INT msec)
{
    m_IsRuning=false;
#ifdef _WIN32
	::WaitForSingleObject(m_Thread_Event,msec);
	//::WaitForSingleObject(m_Thread_Event,1000);
	//::TerminateThread(m_Thread_Id,0);
#else
     pthread_join(m_Thread_Id,NULL);
#endif
}

ST_VOID Thread::SLEEP(ST_INT msec)
{
#if defined(_WIN32)
	Sleep (msec);
#else
    struct timespec intervals = { time_t(msec / 1000L), (msec % 1000L) * 1000000L };
    struct timespec remaining = { 0, 0 };
    while ( nanosleep(&intervals, &remaining) < 0 && errno == EINTR )
    {
        intervals = remaining;
        errno = 0;
    }
#endif
}
