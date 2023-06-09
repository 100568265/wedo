#ifndef ENGINEBASE_H
#define ENGINEBASE_H

#define MAXLINE	1024

#include <vector>
#include "sysgenlist.h"
#include "systhread.h"
#include "syslogger.h"
#include "PortTask.h"
#include "Devices.h"
#include "PortBases.h"
#include "sysqueue.h"


class Channel;

using namespace std;

class EngineBase
{
public:
	enum Type {
		ENGINE_FULLING = 0x0,
		ENGINE_POLLING = 0x1
	};

    EngineBase(Channel *channel);
    virtual ~EngineBase();
    virtual ST_VOID			Init();
    virtual ST_VOID			Uninit();
    virtual ST_VOID			Work()=0;
    virtual ST_VOID			Stop()=0;
	virtual ST_VOID			CheckPort();
	virtual ST_BOOLEAN		IsWorking() ;
	virtual ST_BOOLEAN		SendTask(ProtocolTask &task);
	virtual ST_VOID			ReadTask(PortTask *task);
    virtual ST_VOID			OnTask(ProtocolTask &task,ST_BOOLEAN ignorePort=false);
    virtual ST_VOID			OnTaskBack(const ProtocolTask &task);
	virtual ST_VOID			OnConnect(ST_INT port,ST_UINT64 portAddr);
	virtual ST_VOID			OnDisconnect(ST_INT port,ST_UINT64 portAddr);
	virtual PortBases		*GetPorts();
	virtual Devices			*GetDevices();
	ST_VOID                 DisposeDataCache(ST_UINT64 portAddr);
protected:
	virtual ST_VOID			LoadControlProtocol();
	virtual ST_VOID			LoadDeviceProtocol();
	virtual ST_INT			GetPriorDevice();
	virtual ST_VOID			AddPriorDevice(ST_INT deviceId);
public:
	List<ProtocolBase *>	m_pProtocols;
	PortBuffer				*m_pPortBuffer;
protected:
    Devices					*m_pDevices;
    PortBases				*m_pPorts;
    Channel					*m_pChannel;
    ST_BOOLEAN				m_Inited;
    ST_BOOLEAN				m_Working;
    ST_BOOLEAN				m_sendOk;
    SysLogger				*m_pLogger;
    Queue<PortTask>			m_PortTasks;
	Mutex					m_Mutex;
	ST_VOID					*m_LibCtlHandle;
	ST_VOID					*m_LibAppHandle;
	ProtocolBase			*m_pCtlProtocol;
	ProtocolBase			*m_pAppProtocol;
	vector<ST_INT>			m_priorDevice;
};


#endif
