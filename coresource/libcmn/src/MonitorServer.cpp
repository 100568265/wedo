//#include "stdafx.h"
#include "MonitorServer.h"
//#include "sigaction.h"

MonitorServer::MonitorServer(void)
{
	m_port=NULL;
	m_init=false;
	m_working=false;
}

MonitorServer::~MonitorServer(void)
{
	Uninit();
}

ST_VOID MonitorServer::Init()
{
	if(!m_init) {
		m_PortInfo.LocalPort = 9999;
		Strcpy(m_PortInfo.LocalAddress, "0.0.0.0");
		Strcpy(m_PortInfo.PortName, "monitor");
		m_PortInfo.PortType = PortBase::PORT_TCPSERVER;
		m_port = new PortTcpServerBase(NULL, &m_PortInfo);
		m_port->Init();
/*		#ifdef _WIN32
        #else
            struct sigaction sa;
            sa.sa_handler = SIG_IGN;
            sa.sa_flags=0;
            sigaction(SIGPIPE, &sa, 0 );
        #endif*/
		m_init=true;
	}
}

ST_VOID	MonitorServer::Uninit()
{
	if(NULL != m_port) {
		m_port->Close();
		m_thread.Stop(1000);
		delete m_port;
		m_port = NULL;
	}
	m_init = false;
	m_working = false;
}

ST_VOID MonitorServer::Work()
{
	if(!m_working){
		m_port->Open();
		m_thread.Start(ReceiveProc,m_port,true);
		m_working=true;
	}
}

#ifdef _WIN32
ST_UINT32 MonitorServer::ReceiveProc(ST_VOID *param)
#else
ST_VOID *MonitorServer::ReceiveProc(ST_VOID *param)
#endif
{
    PortBase *pPort=(PortBase*)param;
	if(pPort==NULL) return 0;
    pPort->Recv();
	return 0;
}

ST_VOID MonitorServer::SendMessage(const ST_CHAR *msg, ST_INT channelId, ST_INT deviceId)
{
	int len = Strlen(msg);
	if(m_port && m_port->IsOpened() && len > 0)
	{
		ST_CHAR *tmp = new ST_CHAR[len+8];
		Memcpy(tmp+8, msg, len);
		tmp[0]=(ST_BYTE)0xEB;
		tmp[1]=(ST_BYTE)0x90;
		tmp[2]=(ST_BYTE)channelId;
		tmp[3]=(ST_BYTE)(channelId>>8);
		tmp[4]=(ST_BYTE)deviceId;
        tmp[5]=(ST_BYTE)(deviceId>>8);
 		tmp[6]=(ST_BYTE)len;
        tmp[7]=(ST_BYTE)(len>>8);
		m_port->Send(-1, (ST_BYTE*)tmp, len+8);
		delete[] tmp;
		// Thread::SLEEP(1);
	}
/*	else{
		Thread::SLEEP(100);
	}*/
}
