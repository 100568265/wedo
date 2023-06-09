
#include "PortBase.h"

#include "syslogger.h"

PortBase::PortBase(Channel * channel, PortInfo * portInfo)
{
    m_Inited    = false;
    m_IsOpened  = false;
	m_Working   = false;
    m_pPorts    = NULL;
    m_pChannel  = channel;
    m_pPortInfo = portInfo;
    m_pLogger   = SysLogger::GetInstance();
}

PortBase::~PortBase()
{
   Stop();
   Uninit();
}

ST_VOID PortBase::Init()
{
}

ST_VOID PortBase::Work()
{
	if(!m_Working) {
		m_portThread.Start(ReceiveProc, this, true);
		m_Working = true;
	}
}

ST_VOID PortBase::Stop()
{
	if (m_Working) {
		m_portThread.Stop(1000);
		m_Working = false;
	}
}

ST_VOID PortBase::Close(ST_UINT64 port)
{
}

ST_VOID PortBase::Uninit()
{
}

ST_BOOLEAN PortBase::IsOpened()
{
    return m_IsOpened;
}

Channel *PortBase::GetChannel()
{
    return m_pChannel;
}

#ifdef _WIN32
ST_UINT32 PortBase::ReceiveProc(ST_VOID *param)
#else
ST_VOID * PortBase::ReceiveProc(ST_VOID *param)
#endif
{
	if (param) {
        ((PortBase*)param)->Recv();
    }
	return 0;
}

PortInfo * PortBase::GetPortInfo()
{
    return m_pPortInfo;
}



