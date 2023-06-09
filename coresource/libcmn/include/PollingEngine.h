#ifndef POLLINGENGINE_H
#define POLLINGENGINE_H

#include "sysgenlist.h"
#include "systhread.h"
#include "syslogger.h"
#include "PortTask.h"
#include "Devices.h"
#include "PortBases.h"
#include "EngineBase.h"
#include "sysqueue.h"

class Channel;

class PollingEngine : public EngineBase
{
public:
    PollingEngine(Channel *channel);
    virtual ~PollingEngine();
    virtual ST_VOID 			Work();
    virtual ST_VOID 			Stop();
	virtual ST_VOID				AddPriorDevice(ST_INT deviceId);
	virtual ST_BOOLEAN 			FindNextDevice();
	virtual ST_INT				GetDeviceByIndex(ST_INT devIndex);
	virtual ST_VOID				Recieve(ProtocolBase* protocol,ST_BOOLEAN isResend);
protected:
	virtual ST_VOID				OnWorkTask();
#ifdef _WIN32
	static ST_UINT32 __stdcall	WorkTaskProc(ST_VOID *param);
#else
    static ST_VOID				*WorkTaskProc(ST_VOID *param);
#endif
protected:
	Thread						m_WorkTread;
	ST_INT						m_LinkResendTimes;              //链路重发次数
	ST_INT						m_LinkResendCount;              //链路重发记数
	ST_INT						m_LinkReCheckTimes;				//线路重新检测间隔次数
	ST_INT						m_LinkReCheckCount;				//线路重新检测发记数

	std::map<int, int>			m_breakCountMap;				// map <DeviceID, Count>

	ST_INT 						m_priorIndex;
	ST_INT 						m_continueIndex;
	ST_INT 						m_curDevIndex;
	ST_INT 						m_resendCount;
	ProtocolBase				*m_pProtocl;
	ProtocolBase                *m_pResendProtocl;
};

#endif // POLLINGENGINE_H
