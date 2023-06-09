//#include "stdafx.h"
#include "MonitorClient.h"

MonitorClient::MonitorClient()
{
	m_pLogger=SysLogger::GetInstance();
	m_init=false;
}

MonitorClient::~MonitorClient(void)
{
	Uninit();
}


ST_VOID MonitorClient::Init()
{
	if(!m_init){
		m_PortInfo.RemotePort=9999;
		Strcpy(m_PortInfo.RemoteAddress,"0.0.0.0");
		Strcpy(m_PortInfo.PortName,"monitorclient");
		m_PortInfo.PortType=3;
		m_port=new PortTcpClientBase(NULL,&m_PortInfo);
		m_port->m_client=this;
		m_port->Init();
		m_thread.Start(ReceiveProc,m_port,true);
		m_init=true;
	}
}

ST_VOID	MonitorClient::Uninit()
{
	if(NULL!=m_port){
		m_port->Close();
		m_thread.Stop(1000);
		delete m_port;
		m_port=NULL;
	}
	m_init=false;
	m_working=false;
}

ST_VOID MonitorClient::Connect(ST_CHAR remoteAddr[65])
{
	Strcpy(m_PortInfo.RemoteAddress,remoteAddr);
	m_port->Init();
	m_port->Open();
}

ST_BOOLEAN MonitorClient::IsConnected()
{
	return m_port->IsOpened();
}

ST_VOID MonitorClient::DisConnect()
{
	if(m_init){
		m_port->Close();
	}
}

ST_VOID MonitorClient::ReadTask(PortTask &task)
{
	ST_BYTE *ptr=task.GetBuffer();
	if(task.GetBufferLen()>4){
		int bufLen=task.GetBufferLen()-1;
		ST_CHAR *buf=new ST_CHAR[bufLen];
		int channelId=*ptr++;
		channelId = 256*(*ptr);
		ptr++;
		int deviceId=*ptr++;
		deviceId = 256*(*ptr);
		ptr++;
		Memset(buf,0x00,bufLen);
		Memcpy(buf,ptr,bufLen-1);
		OnShowMessage(buf,channelId,deviceId);
		delete[] buf;
	}
}

ST_VOID MonitorClient::OnShowMessage(ST_CHAR *msg, ST_INT channelId, ST_INT deviceId)
{
	try{
		if(m_ShowMessageBackEx != NULL) {

			m_ShowMessageBackEx(msg, channelId, deviceId, m_pCallObj);
		}
	}
	catch(...) {
		m_pLogger->LogDebug("exception:OnShowMessage%s", msg);
	}
}

ST_VOID MonitorClient::RegistShowMessageBackEx(ShowMessageBackEx showMsgFunc,ST_VOID *callObj)
{
    m_pCallObj=callObj;
    m_ShowMessageBackEx=showMsgFunc;
}

#ifdef _WIN32
ST_UINT32 MonitorClient::ReceiveProc(ST_VOID *param)
#else
ST_VOID *MonitorClient::ReceiveProc(ST_VOID *param)
#endif
{
    PortBase *pPort=(PortBase*)param;
	if(pPort==NULL) return 0;
    pPort->Recv();
	return 0;
}
