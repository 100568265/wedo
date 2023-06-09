//#include "stdafx.h"
#include "PortBases.h"
#include "Channel.h"

PortBases::PortBases(Channel *channel)
{
    m_pMainPort=NULL;
    m_pSlavPort=NULL;
    m_pChannel=channel;
    m_ConnectTimes=0;
    m_pLogger=SysLogger::GetInstance();
}

PortBases::~PortBases()
{

}

ST_VOID PortBases::Init()
{
    m_pMainPort=m_pChannel->GetMainPort();
    if(NULL!=m_pMainPort){
        m_pMainPort->Init();
        m_pLogger->LogDebug("MainPort init success!");
    }
    m_pSlavPort=m_pChannel->GetSlavPort();
    if(NULL!=m_pSlavPort){
        m_pSlavPort->Init();
        m_pLogger->LogDebug("SlavPort init success!");
    }
}

ST_VOID PortBases::Uninit()
{
	if(NULL!=m_pMainPort){
		delete m_pMainPort;
		m_pMainPort=NULL;
	}
	if(NULL!=m_pSlavPort){
		delete m_pSlavPort;
		m_pSlavPort=NULL;
	}
}

ST_VOID PortBases::Work()
{
    if(NULL!=m_pMainPort){
        m_pMainPort->Work();
    }
    if(NULL!=m_pSlavPort){
        m_pSlavPort->Work();
    }
}

ST_VOID PortBases::Stop()
{
   if(NULL!=m_pMainPort){
        m_pMainPort->Stop();
    }
    if(NULL!=m_pSlavPort){
        m_pSlavPort->Stop();
    }
}


ST_BOOLEAN PortBases::Send(ST_BYTE *pBuf,ST_INT len,ST_UINT64 dstAddr)
{
    m_sendOk = false;
    if(m_pMainPort!=NULL)
    {
        if(!m_pMainPort->IsOpened())
        {
            m_pMainPort->Open();
            sleep(50);
        }
        if(m_pMainPort->IsOpened())
            m_sendOk=m_pMainPort->Send(dstAddr,pBuf,len);
    }
    if(!m_sendOk && (m_pSlavPort!=NULL && m_pSlavPort->IsOpened())) {
        m_sendOk=m_pSlavPort->Send(dstAddr,pBuf,len);
    }
    return m_sendOk;
}





ST_VOID PortBases::Close()
{
    if(NULL!=m_pMainPort){
        m_pMainPort->Close();
    }
    if(NULL!=m_pSlavPort){
        m_pSlavPort->Close();
    }
}

ST_VOID PortBases::Close(ST_UINT64 port)
{
    if(NULL!=m_pMainPort){
        m_pMainPort->Close(port);
    }
    if(NULL!=m_pSlavPort){
        m_pSlavPort->Close(port);
    }
}

ST_BOOLEAN PortBases::IsOpened()
{
    if(NULL!=m_pMainPort){
        return m_pMainPort->IsOpened();
    }
    if(NULL!=m_pSlavPort){
        return m_pSlavPort->IsOpened();
    }
    return  false;
}

ST_BOOLEAN PortBases::CanRemoteCtrl()
{
    if (m_pMainPort)
        return m_pMainPort->GetPortInfo()->IsRemoteCtrl;
    if (m_pSlavPort)
        return m_pSlavPort->GetPortInfo()->IsRemoteCtrl;

    return false;
}

ST_VOID PortBases::Open()
{
    if(NULL!=m_pMainPort && !m_pMainPort->IsOpened()){
        m_pMainPort->Open();
    }
    if(NULL!=m_pSlavPort && !m_pSlavPort->IsOpened()){
        m_pSlavPort->Open();
    }
}

PortBase *PortBases::GetMainPort()
{
	return m_pMainPort;
}

PortBase *PortBases::GetSlavPort()
{
	return m_pSlavPort;
}

Channel *PortBases::GetChannel()
{
	return m_pChannel;
}
