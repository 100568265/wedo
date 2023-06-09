#ifndef MONITORSERVER_H_INCLUDE
#define MONITORSERVER_H_INCLUDE
#include "ChannelConfig.h"
#include "PortBase.h"
#include "PortTcpServerBase.h"

class MonitorServer
{
public:
	MonitorServer();
	virtual ~MonitorServer();
	ST_VOID Init();
	ST_VOID Uninit();
	ST_VOID	Work();
	ST_VOID SendMessage(const ST_CHAR *msg, ST_INT channelId, ST_INT deviceId);
#ifdef _WIN32
	static ST_UINT32 __stdcall	ReceiveProc(ST_VOID *param);
#else
    static ST_VOID				*ReceiveProc(ST_VOID *param);
#endif
private:
	PortTcpServerBase *m_port;
	PortInfo m_PortInfo;
	Thread	m_thread;
	ST_BOOLEAN m_init;
	ST_BOOLEAN m_working;
};


#endif //MONITORSERVER_H_INCLUDE