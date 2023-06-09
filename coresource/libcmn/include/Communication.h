#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <signal.h>
#include "sysgenlist.h"
#include "datatype.h"
#include "systhread.h"
#include "Channel.h"
#include "GlobalCfg.h"
#include "ChannelConfig.h"


class SysLogger;
class Dispatcher;
class ProtocolBase;
class MonitorServer;
class ActualValueServer;
#ifdef _WIN32
class __declspec(dllexport) Mutex;
class COMMUNICATIONDLL_API Communication
#else
class Communication
#endif
{
public:
    Communication();
	virtual ~Communication();

	virtual ST_VOID			    Init();
	virtual ST_VOID				Init(ST_INT channelId,ProtocolBase *protocol);
	virtual ST_VOID				Uninit();
	virtual ST_VOID				Work();
	virtual ST_VOID				Stop();
	virtual ST_INT				AddChannel(ChannelInfoA* channel,PortInfoA *mainPort,PortInfoA *slavPort);
	virtual ST_INT				AddDevice(ST_INT channelId,DeviceInfoA* deviceInfo);
	virtual ST_INT				RegistProtocol(ST_INT channelId,ST_INT deviceId,ST_VOID *protocol);
	virtual ST_VOID				ExcCommand(ProtocolTask &task,ST_BOOLEAN ignorePort=false) ;
	virtual ST_VOID				OnTaskBack(const ProtocolTask &task);
	virtual ST_BOOLEAN			IsWorking();
	virtual Dispatcher		   *GetEngine();
	virtual List<Channel>	   *GetChannels();
	virtual Channel			   *GetChannel(ST_INT channelId);
	virtual ST_VOID				OnShowMessage(const ST_CHAR *msg, ST_INT channelId, ST_INT deviceId);

    ST_VOID						RegistProtocolTaskBack(ProtocolTaskBack backFunc);
    ST_VOID						RegistShowMessageBackEx(ShowMessageBackEx showMsgFunc,ST_VOID *callObj);
	ST_VOID						RegistShowMessageBack(ShowMessageBack showMsgFunc);
	ST_VOID						RegistSourceView(ST_INT channelId,ST_BOOLEAN enable);
protected:
    // virtual ST_VOID				IngoreSignal();
    virtual ST_VOID				ExcCommand(ST_INT channelId,ProtocolTask &task,ST_BOOLEAN ignorePort=false);
    virtual List<Channel>	   *CreateChannels();
	virtual Dispatcher		   *CreateEngine();
protected:
    Mutex						m_Mutex;
	ST_BOOLEAN			        m_Inited;
	ST_BOOLEAN			        m_Working;
    std::string					m_Msg;
    SysLogger			       *m_pLogger;
	List<Channel>			   *m_Channels;
	List<ChannelInfo>		   *m_ChannelInfos;
    Dispatcher				   *m_pDEngine;
    ProtocolTaskBack			m_ProtocolTaskBack;
    ShowMessageBackEx			m_ShowMessageBackEx;
	ShowMessageBack				m_ShowMessageBack;
    ST_VOID					   *m_pCallObj;
	MonitorServer			   *m_pMonitor;
	ActualValueServer          *m_pActual;
};

#endif // COMMUNICATION_H
