//#include "stdafx.h"
#include "EngineBase.h"
#include "Channel.h"
#include "Dispatcher.h"
#include "Communication.h"

EngineBase::EngineBase(Channel *channel)
{
    m_Inited  = false;
    m_Working = false;
    m_pChannel= channel;
    m_pPorts  = new PortBases(m_pChannel);
    m_pDevices= new Devices(m_pChannel);
    m_pLogger = SysLogger::GetInstance();
	m_pPortBuffer = new PortBuffer();

	m_LibCtlHandle = NULL;
    m_pCtlProtocol = NULL;
	m_pAppProtocol = NULL;
	m_LibAppHandle = NULL;
}

EngineBase::~EngineBase()
{
	Uninit();

	delete m_pPortBuffer;
}

ST_VOID EngineBase::Init()
{
    if(m_Inited) return;
	m_pPorts->Init();
	LoadControlProtocol();
	if(NULL != m_pCtlProtocol) {
        m_pCtlProtocol->Init();
		m_pProtocols.Add(&m_pCtlProtocol);
    }
/*	LoadDeviceProtocol();//linweiming
	if(NULL != m_pAppProtocol) {
        m_pAppProtocol->Init();
		m_pProtocols.Add(&m_pAppProtocol);
    }*/
    m_pDevices->Init();
    m_Inited = true;
}

ST_VOID EngineBase::Uninit()
{
  if(!m_Inited) return;
	if(NULL != m_pCtlProtocol) {
		m_pProtocols.Remove(&m_pCtlProtocol);
		m_pCtlProtocol->Uninit();
	}
	//if(NULL!=m_pAppProtocol){
	//	m_pProtocols.Remove(&m_pAppProtocol);
	//	m_pAppProtocol->Uninit();
	//}
#ifdef _WIN32
	FreeLibrary((HMODULE)m_LibCtlHandle);
	//FreeLibrary((HMODULE)m_LibAppHandle);
#else
    if(NULL != m_LibCtlHandle) {
        dlclose(m_LibCtlHandle);
        m_LibCtlHandle = NULL;
    }
	if(NULL != m_LibAppHandle) {
        dlclose(m_LibAppHandle);
        m_LibAppHandle=NULL;
    }
#endif
	if(NULL != m_pDevices) {
		delete m_pDevices;
		m_pDevices = NULL;
	}
	if(NULL != m_pPorts) {
		delete m_pPorts;
		m_pPorts = NULL;
	}
   /* m_pPorts->Uninit();
    m_pDevices->Uninit();*/
    m_Inited = false;
}



ST_BOOLEAN EngineBase::IsWorking()
{
    return m_Working;
}

ST_VOID EngineBase::CheckPort()
{
	if(m_pChannel->GetChannelInfo()->AutoOpen==1){
		if(this->IsWorking()){
			m_pPorts->Open();
		}
		else{
			m_pPorts->Close();
		}

	}
}

ST_VOID	EngineBase::AddPriorDevice(ST_INT deviceId)
{
	m_priorDevice.push_back(deviceId);
}

ST_VOID EngineBase::ReadTask(PortTask *task)
{
	if(NULL==task) return;
	if (!m_pPortBuffer) return;
	if(NULL!=m_pCtlProtocol){
		if(m_pCtlProtocol->OnRead(task)){
			m_pPortBuffer->Write(task);
		}
	}
	else{
		m_pPortBuffer->Write(task);
	}
}

ST_INT EngineBase::GetPriorDevice()
{
	if(m_priorDevice.empty()){
		return -1;
	}
	ST_INT devIndex = m_priorDevice.back();  //得到数组的最后一个单元的引用
	m_priorDevice.pop_back();                //去掉数组的最后一个数据
	return devIndex;
}

ST_VOID EngineBase::DisposeDataCache(ST_UINT64 portAddr)
{
	if (!m_pPortBuffer) return ;
	m_pPortBuffer->DelCache(portAddr);
}

ST_VOID	EngineBase::OnConnect(ST_INT port,ST_UINT64 portAddr)
{
	m_pDevices->OnConnect(port,portAddr);
}

ST_VOID	EngineBase::OnDisconnect(ST_INT port,ST_UINT64 portAddr)
{
	m_pDevices->OnDisconnect(port,portAddr);
}

ST_BOOLEAN EngineBase::SendTask(ProtocolTask &task)
{
	if(task.isTransfer)
	{
		return m_pChannel->GetDEngine()->TransmitTask(task);
    }
	else
	{
		Device *pDevice = m_pDevices->GetDevice(task.deviceId);
		if(NULL != pDevice)
		{
			ProtocolBase *protocol = pDevice->GetProtocol();
			if(NULL == protocol)
			{
				protocol = m_pAppProtocol;
			}
			protocol->OnTask(&task);
		}
		return true;
	}
}

ST_VOID EngineBase::LoadControlProtocol()
{
	ST_CHAR * fileName = m_pChannel->GetChannelInfo()->CtlProtocolFile;
	if(Strlen(fileName) == 0) {
		m_pLogger->LogDebug("Ctl protocol file is null");
		return;
	}
	ST_CHAR fullName[256] = {0};
	Strncat(fullName, ".//protocols//", sizeof(fullName));
	Strncat(fullName,         fileName, sizeof(fullName));
#ifdef _WIN32

    m_LibCtlHandle = LoadLibrary(fullName);
	if (NULL != m_LibCtlHandle) {
		m_pLogger->LogDebug("load %s success!", fileName);
    	CreateProtocol getProtocol = (CreateProtocol)GetProcAddress((HMODULE)m_LibCtlHandle, "CreateInstace");
#else

    m_LibCtlHandle = dlopen(fullName, RTLD_LAZY);
    if (NULL != m_LibCtlHandle) {
        m_pLogger->LogDebug("load Ctl  %s success!", fileName);
        CreateProtocol getProtocol = (CreateProtocol)dlsym(m_LibCtlHandle, "CreateInstace");
#endif
        if(getProtocol) {
            m_pCtlProtocol = getProtocol();
			m_pCtlProtocol->m_pChannel = m_pChannel;
			m_pCtlProtocol->m_pPorts   = m_pPorts;
			m_pCtlProtocol->SetDevice(NULL);
            m_pLogger->LogDebug("Create Ctl protocol success!");
            return;
		}
        else {
            m_pLogger->LogDebug("Get Ctl protocol error");
		}
	}
/*    else{
		#ifndef _WIN32
        m_pLogger->LogDebug("load %s failure. errno: %d, desc: %d", fileName, errno, strerror(errno));
		#endif
	}*/
}

ST_VOID EngineBase::LoadDeviceProtocol()
{
	ST_CHAR * fileName = m_pChannel->GetChannelInfo()->ProtocolFile;
	if(Strlen(fileName) == 0) {
		m_pLogger->LogDebug("Protocol file is null");
		return;
	}
	ST_CHAR fullName[256] = {0};
	Strncat(fullName, ".//protocols//", sizeof(fullName));
	Strncat(fullName,         fileName, sizeof(fullName));
#ifdef _WIN32

	Strncat(fullName, ".dll", sizeof(fullName));
    m_LibAppHandle = LoadLibrary(fullName);
	if (NULL != m_LibAppHandle) {
		m_pLogger->LogDebug("load %s success!", fileName);
    	CreateProtocol getProtocol = (CreateProtocol)GetProcAddress((HMODULE)m_LibAppHandle, "CreateInstace");
#else

	Strncat(fullName,  ".so", sizeof(fullName));
    m_LibAppHandle = dlopen(fullName, RTLD_LAZY);
    if (NULL != m_LibAppHandle) {
        m_pLogger->LogDebug("load %s success!", fileName);
        CreateProtocol getProtocol = (CreateProtocol)dlsym(m_LibAppHandle, "CreateInstace");
#endif
        if(getProtocol) {
            m_pAppProtocol = getProtocol();
			m_pAppProtocol->m_pChannel = m_pChannel;
			m_pAppProtocol->m_pPorts   = m_pPorts;
//			m_pAppProtocol->SetDevice(NULL); ///linweiming
            m_pLogger->LogDebug("Create protocol success!");
			return;
		}
        else {
            m_pLogger->LogDebug("Get protocol error");
		}
	}
}

ST_VOID EngineBase::OnTask(ProtocolTask &task,ST_BOOLEAN ignorePort)
{
	task.taskResult.resultCode = -1;
	if(m_pPorts->IsOpened()){
		Device *pDevice=m_pDevices->GetDevice(task.deviceId);
		if(NULL!=pDevice){
			SendTask(task);
			AddPriorDevice(task.deviceId);
			task.taskResult.resultCode = 0;
		}
	}
	else {
		OnTaskBack(task);
	}
}

ST_VOID EngineBase::OnTaskBack(const ProtocolTask &task)
{
	if(task.ignoreBack > 0)return;
    if(NULL!=m_pChannel->GetCommunication())
	m_pChannel->GetCommunication()->OnTaskBack(task);
}

PortBases *EngineBase::GetPorts()
{
	return m_pPorts;
}

Devices *EngineBase::GetDevices()
{
	return m_pDevices;
}
