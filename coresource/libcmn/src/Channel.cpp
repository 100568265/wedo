//#include "stdafx.h"
#include "Channel.h"
#include "Communication.h"
#include "datetime.h"

#include "MQTT/PortICBC.h"
#include "LGMQTT/PortLGMQTT.h"
#include "LGMQTT/PortMQTTClient.h"

#include <string>
#include <assert.h>

Channel::Channel(Communication *communication, ChannelInfo *channelInfo):
m_pMainPort(NULL),
m_pSlavPort(NULL),
m_pDevices (NULL),
m_pCEngine (NULL),
m_pChannelInfo  (channelInfo),
m_pCommunication(communication),
m_Inited  (false),
m_Working (false),
m_showSourceView (false),
m_ChannelState(0),
m_ChannelBrokenTimes(0)
{
    m_pLogger=SysLogger::GetInstance();
}

Channel::~Channel()
{
   Uninit();
}

ST_VOID Channel::Init()
{
    if(m_Inited) return;
    assert(m_pChannelInfo!=NULL);
    assert(m_pCommunication!=NULL);
    CreateMainPort();
    CreateSlavPort();
    CreateDevices();
    CreateEngine();
    assert(m_pCEngine!=NULL);
    m_pCEngine->Init();
    m_Inited=true;
}

ST_VOID Channel::Uninit()
{
    if(!m_Inited) return;
    assert(m_pCEngine!=NULL);
	if(NULL != m_pCEngine) {
        delete m_pCEngine;
        m_pCEngine = NULL;
    }
    m_Inited = false;
}

ST_VOID Channel::Work()
{
	if(m_Working) return;
    m_pCEngine->Work();
    m_Working = true;
}

ST_VOID Channel::Stop()
{
	m_Working = false;
    m_pCEngine->Stop();
}

PortBase *Channel::GetMainPort()
{
    return m_pMainPort;
}

PortBase *Channel::GetSlavPort()
{
    return m_pSlavPort;
}

Device   *Channel::GetDevice (ST_INT devid)
{
    ST_INT index = (m_pDevices ? m_pDevices->GetCount() : 0);
    while (index --> 0) {
        if (m_pDevices->GetItem (index)->GetId() == devid)
            return m_pDevices->GetItem (index);
    }
    return NULL;
}

List<Device> *Channel::GetDevices()
{
     return m_pDevices;
}

EngineBase *Channel::GetCEngine()
{
    return m_pCEngine;
}

Dispatcher *Channel::GetDEngine()
{
    return m_pCommunication->GetEngine();
}

ST_INT Channel::GetLocalChannelID()
{
    return m_pChannelInfo->ChannelID;
}

ST_INT Channel::GetTransmitChannelID()
{
    return m_pChannelInfo->TransChannelID;
}

ChannelInfo *Channel::GetChannelInfo()
{
    return m_pChannelInfo;
}

Communication *Channel::GetCommunication()
{
    return m_pCommunication;
}

ST_BOOLEAN	Channel::IsWorking()
{
	return m_Working;
}

ST_VOID Channel::CreateEngine()
{
	if(this->GetChannelInfo()->ProtocolType == 1)
    {
		m_pCEngine = new PollingEngine(this);
	}
	else {
		m_pCEngine = new FullingEngine(this);
	}
    assert(m_pCEngine != NULL);
}

ST_VOID Channel::DisposeDataCache(ST_UINT64 portAddr)
{
	if(NULL != m_pCEngine) {
		m_pCEngine->DisposeDataCache(portAddr);
	}
}

ST_VOID Channel::CreateMainPort()
{
    if(NULL == m_pChannelInfo->MainPort) return;
    switch(m_pChannelInfo->MainPort->PortType) {
        case PortBase::PORT_COM:
            m_pMainPort = new PortCom(this, m_pChannelInfo->MainPort);
            break;
        case PortBase::PORT_TCPSERVER:
            m_pMainPort = new PortTcpServer(this, m_pChannelInfo->MainPort);
            break;
        case PortBase::PORT_TCPCLIENT:
            m_pMainPort = new PortTcpClient(this, m_pChannelInfo->MainPort);
            break;
        case PortBase::PORT_UDP:
            m_pMainPort = new PortUdp(this, m_pChannelInfo->MainPort);
            break;
		case PortBase::PORT_ICBC_MQTT:
			m_pMainPort = new MQTT::PortICBC(this, m_pChannelInfo->MainPort);
			break;
        case PortBase::PORT_MQTT:
			m_pMainPort = new PortMQTTClient(this,m_pChannelInfo->MainPort);
			break;
        case PortBase::PORT_CAN:
			m_pMainPort = new PortCan(this, m_pChannelInfo->MainPort);
			break;
    }
}

ST_VOID Channel::CreateSlavPort()
{
    if(NULL == m_pChannelInfo->SlavPort) return;
    switch(m_pChannelInfo->SlavPort->PortType) {
        case PortBase::PORT_COM:
            m_pSlavPort = new PortCom(this, m_pChannelInfo->SlavPort);
            break;
        case PortBase::PORT_TCPSERVER:
            m_pSlavPort = new PortTcpServer(this, m_pChannelInfo->SlavPort);
            break;
        case PortBase::PORT_TCPCLIENT:
            m_pSlavPort = new PortTcpClient(this, m_pChannelInfo->SlavPort);
            break;
        case PortBase::PORT_UDP:
            m_pSlavPort = new PortUdp(this, m_pChannelInfo->SlavPort);
            break;
        case PortBase::PORT_CAN:
			m_pSlavPort = new PortCan(this, m_pChannelInfo->SlavPort);
			break;
        default: return;
    }
    // assert(m_pSlavPort != NULL);
}

ST_VOID Channel::CreateDevices()
{
    Device *m_pDevice = NULL;
    m_pDevices = new List<Device>();
	if(m_pChannelInfo->DeviceInfos == NULL) return;
    for(int i = 0; i < m_pChannelInfo->DeviceInfos->GetCount(); i++)
    {
        m_pDevice = new Device(this, m_pChannelInfo->DeviceInfos->GetItem(i));
        assert(m_pDevice != NULL);
        m_pDevices->Add(m_pDevice);
    }
}

ST_INT	Channel:: GetChannelState()
{
	return m_ChannelState;
}

ST_VOID	Channel::SetChannelState(ST_INT state)
{
	m_ChannelState = state;
	ST_CHAR varName[296] = {0};
	snprintf(varName, sizeof(varName), "channelstate.%s.value", this->GetChannelInfo()->ChannelName);
	ST_VARIANT var;
	var.vt = VALType_UInt32;
	var.iVal = state;
	SetVariableValue (varName, var);
}

ST_VOID Channel::SetChannelBrokenTimes(ST_INT times)
{
	m_ChannelBrokenTimes = times;
}

ST_INT Channel::GetChannelBrokenTimes ()
{
	return m_ChannelBrokenTimes;
}

ST_VOID Channel::ShowMessage(const ST_CHAR *msg, ST_INT deviceId)
{
    if(msg && m_pCommunication != NULL) {
        m_pCommunication->OnShowMessage(msg, m_pChannelInfo->ChannelID, deviceId);
    }
}

static ST_VOID AddHexText (std::string& dest, ST_BYTE *pbuf, ST_INT len)
{
    const static char hxpl [] = "0123456789ABCDEF";
    for (ST_INT i = 0; i < len; ++i) {
        dest += hxpl[pbuf[i] / 16];
        dest += hxpl[pbuf[i] % 16];
        dest += ' ';
    }
}

inline static void GetStrNowTime (ST_CHAR *date)
{
    struct tm tm_now;
    DateTime::localtime (time(0), tm_now);
    strftime(date, 20, "%Y-%m-%d %H:%M:%S", &tm_now);
}

ST_VOID Channel::ShowSendFrame(ST_BYTE *pBuf, ST_INT len, ST_INT deviceId)
{
    if (!IsShowMessage ()) return;
	if (!pBuf || len <= 0) return;

    ST_CHAR date[24] = {0};
    GetStrNowTime (date);

    std::string msg = date;
    msg.reserve (64 + 3 * len);

    msg += "->Send: ";
	AddHexText (msg, pBuf, len);

	ShowMessage(msg.c_str(), deviceId);
}

ST_VOID Channel::ShowRecvFrame(ST_BYTE *pBuf, ST_INT len, ST_INT deviceId)
{
    if (!IsShowMessage ()) return;
	if (!pBuf || len <= 0) return;

    ST_CHAR date[24] = {0};
    GetStrNowTime (date);

    std::string msg = date;
    msg.reserve (64 + 3 * len);

    msg += "->Receive: ";
	AddHexText (msg, pBuf, len);

	ShowMessage(msg.c_str(), deviceId);
}

ST_VOID Channel::RegistSourceView (ST_BOOLEAN showView)
{
	m_showSourceView = showView;
}

ST_BOOLEAN Channel::IsShowMessage ()
{
    return true;
	//return m_showSourceView;
}
