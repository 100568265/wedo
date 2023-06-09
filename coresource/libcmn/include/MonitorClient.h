#ifndef MONITORCLIENT_H_INCLUDE
#define MONITORCLIENT_H_INCLUDE

#include "PortBase.h"
#include "ChannelConfig.h"
#include "PortTcpClientBase.h"
#include "GlobalCfg.h"

#ifdef _WIN32
class __declspec(dllexport) Mutex;
class COMMUNICATIONDLL_API MonitorClient
#else
class MonitorClient
#endif
{
public:
	MonitorClient();
	virtual ~MonitorClient(void);
	ST_VOID Init();
	ST_VOID Uninit();
	ST_BOOLEAN IsConnected();
	ST_VOID	Connect(ST_CHAR remoteAddr[65]);
	ST_VOID DisConnect();
	ST_VOID ReadTask(PortTask &task);
	ST_VOID OnShowMessage(ST_CHAR *msg, ST_INT channelId, ST_INT deviceId);
	ST_VOID	RegistShowMessageBackEx(ShowMessageBackEx showMsgFunc,ST_VOID *callObj);
#ifdef _WIN32
	static ST_UINT32 __stdcall	ReceiveProc(ST_VOID *param);
#else
    static ST_VOID				*ReceiveProc(ST_VOID *param);
#endif
private:
	ShowMessageBackEx m_ShowMessageBackEx;
	PortTcpClientBase *m_port;
	PortInfo m_PortInfo;
	Thread	m_thread;
	ST_BOOLEAN m_init;
	ST_BOOLEAN m_working;
	ST_CHAR m_remoteAddr[65];
	ST_VOID	 *m_pCallObj;
	SysLogger   *m_pLogger;
};


#endif //MONITORCLIENT_H_INCLUDE