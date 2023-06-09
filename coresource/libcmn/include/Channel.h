#ifndef CHANNEL_H
#define CHANNEL_H

#include "datatype.h"
#include "sysgenlist.h"
#include "syslogger.h"
#include "Device.h"
#include "PortBase.h"
#include "PortCom.h"
#include "PortCan.h"
#include "PortUdp.h"
#include "PortTcpClient.h"
#include "PortTcpServer.h"
#include "ChannelConfig.h"
#include "EngineBase.h"
#include "FullingEngine.h"
#include "PollingEngine.h"
#include "Timer.h"

class Communication;

class Channel
{
public:
    Channel(Communication *communication,ChannelInfo *channelInfo);
    virtual ~Channel();
    virtual ST_VOID		Init();
    virtual ST_VOID		Uninit();
    virtual ST_VOID		    Work();
    virtual ST_VOID		    Stop();
    virtual ST_BOOLEAN		IsWorking();

    virtual PortBase		*GetMainPort();
    virtual PortBase		*GetSlavPort();

    virtual Device          *GetDevice (ST_INT devid);
    virtual List<Device>	*GetDevices();

    virtual EngineBase		*GetCEngine();
    virtual Dispatcher		*GetDEngine();

    virtual ChannelInfo	    *GetChannelInfo();
    virtual ST_INT			GetLocalChannelID();
    virtual ST_INT			GetTransmitChannelID();
    virtual Communication	*GetCommunication();
	virtual ST_VOID		    RegistSourceView(ST_BOOLEAN showView);
	ST_BOOLEAN		        IsShowMessage();
	ST_VOID					ShowMessage(const ST_CHAR *pMsg,ST_INT deviceId=-1);
    ST_VOID					ShowSendFrame(ST_BYTE *pBuf,ST_INT len,ST_INT deviceId=-1);
    ST_VOID					ShowRecvFrame(ST_BYTE *pBuf, ST_INT len,ST_INT deviceId=-1);
	ST_VOID                 DisposeDataCache(ST_UINT64 portAddr);

	ST_VOID					SetChannelState(ST_INT state);
	ST_INT					GetChannelState();

	ST_VOID					SetChannelBrokenTimes(ST_INT times);
	ST_INT					GetChannelBrokenTimes();
protected:
    virtual ST_VOID		CreateEngine();
    virtual ST_VOID		CreateDevices();
    virtual ST_VOID		CreateMainPort();
    virtual ST_VOID		CreateSlavPort();
protected:
    PortBase				*m_pMainPort;
    PortBase				*m_pSlavPort;
    List<Device>			*m_pDevices;
    EngineBase				*m_pCEngine;
    ChannelInfo				*m_pChannelInfo;
    Communication			*m_pCommunication;
	SysLogger				*m_pLogger;
    ST_BOOLEAN				 m_Inited;
    ST_BOOLEAN				 m_Working;
	ST_BOOLEAN				 m_showSourceView;
    ST_INT					 m_ChannelState;
	ST_INT					 m_ChannelBrokenTimes;
public:
	ST_CHAR					m_Msg[2048];
};

#endif // CHANNEL_H
