//#include "stdafx.h"
#include "Devices.h"
#include "Channel.h"

Devices::Devices(Channel *channel):
m_pDevices(NULL),
m_Working(false),
m_pChannel(channel)
{
	m_pLogger=SysLogger::GetInstance();
}

Devices::~Devices()
{
	Uninit();
}

ST_VOID Devices::Init()
{
    m_pDevices=m_pChannel->GetDevices();
    for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
        m_pDevice->Init();
    }
}

ST_VOID Devices::Uninit()
{
   if(NULL!=m_pDevices){
        m_pDevices->Clear();
        delete m_pDevices;
        m_pDevices=NULL;
	}
}

ST_VOID Devices::Work()
{
	if(m_Working) return;
    for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
        m_pDevice->Work();
    }
	m_Working=true;
}

ST_VOID Devices::Stop()
{
	if(!m_Working) return;
    for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
        m_pDevice->Stop();
    }
	m_Working=false;
}

ST_INT Devices::GetCount()
{
	return m_pDevices->GetCount();
}

ST_VOID	Devices::OnConnect(ST_INT port,ST_UINT64 portAddr)
{
	 for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
		if(NULL==m_pDevice) continue;
		m_pDevice->OnConnect(port,portAddr);
    }
}

ST_VOID	Devices::OnDisconnect(ST_INT port,ST_UINT64 portAddr)
{
	for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
		if(NULL==m_pDevice) continue;
		m_pDevice->OnDisconnect(port,portAddr);
    }
}

Device *Devices::GetDevice(ST_INT deviceId)
{
    for(int i=0;i<m_pDevices->GetCount();i++){
        Device *m_pDevice=m_pDevices->GetItem(i);
		if(NULL==m_pDevice) return NULL;
        if(m_pDevice->GetDeviceInfo()->DeviceId==deviceId){
            return m_pDevice;
        }
    }
    return NULL;
}

Device *Devices::GetDeviceByAddr(ST_INT deviceAddr)
{
	for(int i=0;i<m_pDevices->GetCount();i++){
        Device *pDevice=m_pDevices->GetItem(i);
		if(pDevice->GetAddress()==deviceAddr){
            return pDevice;
        }
    }
    return NULL;
}

