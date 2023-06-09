//#include "stdafx.h"
#include "Device.h"
#include "Channel.h"

#include <assert.h>
#include <stdio.h>

Device::Device(Channel *channel, DeviceInfo *deviceInfo):
m_pChannel(channel),
m_pDeviceInfo(deviceInfo),
m_pCEngine (NULL),
m_pProtocol(NULL),
m_Inited  (false),
m_Working (false),
m_LibHandle(NULL)
{
    m_pLogger   = SysLogger::GetInstance();
}

Device::~Device()
{
	Stop();
	Uninit();
}

ST_VOID Device::Init()
{
    if(m_Inited) return;
    if(NULL == m_pCEngine) {
        m_pCEngine = m_pChannel->GetCEngine();
    }
    assert(m_pCEngine != NULL);
    LoadProtocol();
    if(NULL != m_pProtocol) {
        m_pProtocol->Init();
    }
    m_Inited = true;
}

ST_VOID Device::Uninit()
{
    if(!m_Inited) return;
    if(NULL != m_pProtocol) {
        m_pProtocol->Uninit();
    }
	m_pCEngine->m_pProtocols.Remove(&m_pProtocol);
#ifdef _WIN32
	FreeLibrary((HMODULE)m_LibHandle);
#else
    if(NULL != m_LibHandle) {
        dlclose(m_LibHandle);
        m_LibHandle = NULL;
    }
#endif
    m_Inited = false;
}

ST_VOID Device::Work()
{
	if(m_Working) return;
    if(NULL != m_pProtocol) {
        m_pProtocol->Start();
    }
	m_Working = true;
}

ST_VOID Device::Stop()
{
	if(!m_Working) return;
    if(NULL != m_pProtocol) {
        m_pProtocol->Stop();
    }
	m_Working = false;
}

ProtocolBase *Device::GetProtocol()
{
    return m_pProtocol;
}

ST_VOID Device::LoadProtocol()
{
    /*
	if(m_pDeviceInfo->Protocol != NULL) {
		goto next;
	}*/
	ST_CHAR * fileName = m_pDeviceInfo->ProtocolFile;
	if(Strlen(fileName) == 0) {
		fileName = m_pChannel->GetChannelInfo()->ProtocolFile;
		if(Strlen(fileName) == 0)
			return;
	}
	//if(Strcmp(m_pDeviceInfo->ProtocolFile,m_pChannel->GetChannelInfo()->ProtocolFile)==0){
	//	return;
	//}
	ST_CHAR fullName[256] = {0};
#ifdef _WIN32

	Strcpy (fullName, ".//protocols//");
	Strncat(fullName, fileName, sizeof (fullName));
	Strncat(fullName,   ".dll", sizeof (fullName));
    m_LibHandle = LoadLibrary(_T(fullName));
	if (NULL != m_LibHandle) {
		m_pLogger->LogDebug("load %s success!", fileName);
    	CreateProtocol getProtocol = (CreateProtocol)GetProcAddress((HMODULE)m_LibHandle,"CreateInstace");
#else

	Strcpy (fullName, ".//protocols//");
	Strncat(fullName, fileName, sizeof (fullName));
	Strncat(fullName,    ".so", sizeof (fullName));
    m_LibHandle = dlopen(fullName,RTLD_LAZY);
    if (NULL != m_LibHandle) {
        m_pLogger->LogDebug("load %s success!", fileName);
        CreateProtocol getProtocol = (CreateProtocol) dlsym (m_LibHandle,"CreateInstace");
#endif
        if(getProtocol)
        {
//next:
//			m_pProtocol=(ProtocolBase*)m_pDeviceInfo->Protocol; //linweiming
            m_pProtocol = getProtocol();
			m_pProtocol->m_pChannel = m_pChannel;
			m_pProtocol->m_pPorts   = GetPorts();
            m_pProtocol->SetDevice(this);
            if(!m_pProtocol->IsSupportEngine(m_pChannel->GetChannelInfo()->ProtocolType)) {
            	printf ("[Error] %s does not support this engine type!\n"
            		"\tPlease check that the protocol is configured with the available channel type.\n", m_pDeviceInfo->ProtocolFile);

            	m_pLogger->LogError("%s does not support this engine type!", m_pDeviceInfo->ProtocolFile);
            	return;
            }
			m_pCEngine->m_pProtocols.Add(&m_pProtocol);
            m_pLogger->LogDebug("create %s success!", fileName);
        }
        else {
            m_pLogger->LogError ("get %s error!", m_pDeviceInfo->ProtocolFile);
        }
    }
    else {
    	printf ("[Error] Load %s dll failed!\n"
    		"\tPlease check if the file exists or is available.\n", fileName);

#ifdef _WIN32
		m_pLogger->LogError("load %s failed. errno[%d]", fileName, WSAGetLastError());
#else
        m_pLogger->LogError("load %s failed. desc[%s]", fileName, dlerror());
#endif
    }
}

Channel *Device::GetChannel()
{
    return m_pChannel;
}

EngineBase *Device::GetEngine()
{
    return m_pCEngine;
}

PortBases *Device::GetPorts()
{
	return m_pCEngine->GetPorts();
}

DeviceInfo *Device::GetDeviceInfo()
{
    return m_pDeviceInfo;
}

ST_INT Device::GetAddress()
{
	if (GetDeviceInfo())
		return GetDeviceInfo()->Address;
	else
		return -1;
}

ST_INT Device::GetId()
{
	if (GetDeviceInfo())
		return GetDeviceInfo()->DeviceId;
	else
		return -1;
}

TransferTables &Device::TransTables()
{
	return m_transTable;
}

ST_VOID	Device::OnConnect(ST_INT port,ST_UINT64 portAddr)
{
	if(NULL == m_pProtocol) return;
	m_pProtocol->OnConnect(port, portAddr);
}

ST_VOID	Device::OnDisconnect(ST_INT port,ST_UINT64 portAddr)
{
	if(NULL == m_pProtocol) return;
	m_pProtocol->OnDisconnect(port,portAddr);
}

