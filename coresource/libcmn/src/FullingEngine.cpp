//#include "stdafx.h"
#include "FullingEngine.h"
#include "Channel.h"
#include "Dispatcher.h"
#include "Communication.h"

class FullingEnginePrivate
{
public:
	FullingEnginePrivate() : curindex(0) {}

	long curindex;
};

FullingEngine::FullingEngine(Channel *channel):
EngineBase(channel),
_p(new FullingEnginePrivate())
{
    m_Inited  = false;
    m_Working = false;
}

FullingEngine::~FullingEngine()
{
	// m_pLogger->LogDebug("~FullingEngine()");
	Stop();
	Uninit();

	delete _p;
}

ST_VOID FullingEngine::Work()
{
	if(m_Working) return;

	m_pDevices->Work();
    m_pPorts->Work();
	m_ReadThread.Start(ReadTaskProc, this, true);
    m_SendThread.Start(SendTaskProc, this, true);
    m_Working = true;
}

ST_VOID FullingEngine::Stop()
{
	if(m_Working) {
		m_ReadThread.Stop();
		m_SendThread.Stop();
		m_pPorts->Stop();
		m_pDevices->Stop();
		m_Working = false;
	}
}

ST_VOID FullingEngine::OnReadTask()
{
	if (!m_Working || m_pProtocols.GetCount() <= 0)
	{
		Thread::SLEEP(30);
		return;
	}

	for(int i = 0; i < m_pProtocols.GetCount() && m_Working; ++i)
	{
		ST_INT received = 0;
		ProtocolBase *curProtocol = *m_pProtocols.GetItem(i);
		try {
			curProtocol->OnRead(curProtocol->m_Buffer, received);
		}
		catch(...) {}

		if(received > 0) {
		    if (m_pChannel->IsShowMessage()) {
				m_pChannel->ShowRecvFrame(curProtocol->m_Buffer, received, curProtocol->GetDevice()->GetDeviceInfo()->DeviceId);
			}
			m_pChannel->SetChannelState(1);
			curProtocol->UpdateDeiveState(1);
			try {
				curProtocol->OnProcess(curProtocol->m_Buffer, received);
			}
			catch(...) {}
		}
	}
}

ST_VOID FullingEngine::OnSendTask()
{
	if (!m_Working )//||m_pProtocols.GetCount() <= 0)    linweiming
	{
		Thread::SLEEP(30);
		return;
	}

	long interval = m_pChannel->GetChannelInfo()->ChannelInterval;
	if (interval > 0) Thread::SLEEP(interval);
	ProtocolBase * protobase = NULL;

	int priorDevId = this->GetPriorDevice();
	if (priorDevId >= 0) {
		long count = m_pChannel->GetDevices()->GetCount();
		for(long i = 0; i < count; ++i) {
			if (m_pChannel->GetDevices()->GetItem(i) != NULL &&
					m_pChannel->GetDevices()->GetItem(i)->GetId() == priorDevId)
				protobase = m_pChannel->GetDevices()->GetItem(i)->GetProtocol();
		}
	}

	if(!protobase) {
        Device * de = m_pChannel->GetDevices()->GetItem(_p->curindex);  //linweiming
        if(de)
            protobase = de->GetProtocol();
	}

	_p->curindex = ++(_p->curindex) % m_pChannel->GetDevices()->GetCount();

	try {
		if (protobase)
		{
			protobase->OnSend();
        }
	}
	catch (std::exception& e) {

	}
	catch (...) {

	}
	// ST_UINT32 interval = m_pChannel->GetChannelInfo()->ChannelInterval;

	// for(int i = 0; i < m_pProtocols.GetCount() && m_Working; ++i)
	// {
	// 	if (interval > 0) Thread::SLEEP(interval);

	// 	ProtocolBase *curProtocol = *m_pProtocols.GetItem(i);
	// 	curProtocol->OnSend();
	// }
}


#ifdef _WIN32
ST_UINT32 FullingEngine::ReadTaskProc(ST_VOID *param)
#else
ST_VOID *FullingEngine::ReadTaskProc(ST_VOID *param)
#endif
{
    FullingEngine *pd = (FullingEngine*)param;
    pd->OnReadTask();
	return 0;
}

#ifdef _WIN32
ST_UINT32 FullingEngine::SendTaskProc(ST_VOID *param)
#else
ST_VOID *FullingEngine::SendTaskProc(ST_VOID *param)
#endif
{
    FullingEngine *pd = (FullingEngine*)param;
	pd->CheckPort();
    pd->OnSendTask();
	return 0;
}
