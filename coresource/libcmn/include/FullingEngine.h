#ifndef __FULLINGENGINE_H__
#define __FULLINGENGINE_H__

#include "sysgenlist.h"
#include "systhread.h"
#include "syslogger.h"
#include "PortTask.h"
#include "Devices.h"
#include "PortBases.h"
#include "EngineBase.h"
#include "sysqueue.h"

class Channel;

class FullingEngine : public EngineBase
{
public:
    FullingEngine(Channel *channel);
    virtual ~FullingEngine();
    virtual ST_VOID				Work();
    virtual ST_VOID				Stop();
protected:
	virtual ST_VOID				OnReadTask();
    virtual ST_VOID				OnSendTask();
#ifdef _WIN32
	static ST_UINT32 __stdcall	ReadTaskProc(ST_VOID *param);
    static ST_UINT32 __stdcall	SendTaskProc(ST_VOID *param);
#else
    static ST_VOID				*ReadTaskProc(ST_VOID *param);
    static ST_VOID				*SendTaskProc(ST_VOID *param);
#endif
protected:
	Thread						m_ReadThread;
    Thread						m_SendThread;

    class FullingEnginePrivate * _p;
};

#endif // __FULLINGENGINE_H__
