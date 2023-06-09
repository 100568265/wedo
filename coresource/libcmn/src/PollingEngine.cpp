//#include "stdafx.h"
#include "PollingEngine.h"
#include "Channel.h"
#include "Dispatcher.h"
#include "Communication.h"

PollingEngine::PollingEngine(Channel *channel)
	:EngineBase(channel)
{
    m_Inited           = false;
    m_Working          = false;
	m_continueIndex    = -1;
	m_curDevIndex      = -1;
	m_priorIndex       = -1;
	m_resendCount      =  0;
	m_LinkResendCount  =  0;
	m_LinkReCheckCount =  0;
	m_pResendProtocl   = NULL;
	m_pProtocl         = NULL;
}

PollingEngine::~PollingEngine()
{
	Stop();
	Uninit();
}

ST_VOID	PollingEngine::AddPriorDevice(ST_INT deviceId)
{
	m_priorDevice.push_back(deviceId);
}

ST_BOOLEAN PollingEngine::FindNextDevice()
{
	bool bRun = false;
	try {
		do {
			if(this->m_continueIndex >= 0) {
				m_curDevIndex = this->m_continueIndex;
				this->m_continueIndex = -1;           //有正确index后赋值并退出
				return true;
			}
			int m_priorIndex = this->GetPriorDevice();
			if (m_priorIndex >= 0)
			{
				m_priorIndex = this->GetDeviceByIndex(m_priorIndex);
				if(m_priorIndex >= 0)
				{
					m_curDevIndex = m_priorIndex;
					return true;
				}
			}
			if (m_pChannel->GetDevices()->GetCount() <= 0) {
				m_curDevIndex = -1;
				return false;
			}
			// m_curDevIndex++;
			if(++m_curDevIndex >= m_pChannel->GetDevices()->GetCount())
			{
				m_curDevIndex = 0;
			}
		    bRun = m_pChannel->GetDevices()->GetItem(m_curDevIndex)->GetDeviceInfo()->IsRun;
		} while((m_curDevIndex == -1) || (bRun == false));
    }
	catch(...)
	{
		if(m_pChannel->IsShowMessage()) {
			m_pChannel->ShowMessage("Engine exception!");
			Thread::SLEEP(2000);
		}
		m_pLogger->LogWarn("[Exception][For \"%s\" Channel]\r\n\tPolling Engine - Find Next Device",
			m_pChannel->GetChannelInfo()->ChannelName);
		return false;
	}
	return true;
}

ST_INT PollingEngine::GetDeviceByIndex(ST_INT devIndex)
{
	ST_INT count = m_pChannel->GetDevices()->GetCount();
	for(ST_INT i = 0 ; i < count; ++i) {
		if (m_pChannel->GetDevices()->GetItem(i) != NULL &&
			m_pChannel->GetDevices()->GetItem(i)->GetId() == devIndex)
			return i;
	}
	return -1;
}

ST_VOID PollingEngine::Work()
{
	if(m_Working) return;
	m_pDevices->Work();
	m_pPorts->Work();
	m_WorkTread.Start(WorkTaskProc,this,true);
    m_Working=true;
}

ST_VOID PollingEngine::Stop()
{
	if(m_Working){
		m_WorkTread.Stop();
		m_pPorts->Stop();
		m_pDevices->Stop();
		m_Working=false;
	}
}


ST_VOID PollingEngine::Recieve(ProtocolBase* protocol,ST_BOOLEAN isResend)
{
	int readed = MAXBUFFERLEN;
	try {
		protocol->OnRead(protocol->m_Buffer, readed);
		if(readed <= 0 || readed == MAXBUFFERLEN)
		{
			if(!isResend)
			{
				m_pResendProtocl = protocol;
				m_resendCount = 0;
			}
		}
		else {
			if (m_pChannel->IsShowMessage()) { //当配置工具连接管理机时，将onRead()判断正确的报文发送到管理机配置工具的报文通道
				m_pChannel->ShowRecvFrame(protocol->m_Buffer, readed, protocol->GetDevice()->GetDeviceInfo()->DeviceId);
			}
			protocol->OnProcess(protocol->m_Buffer, readed);
			if(readed > 0)
			{
				int deviceId = protocol->GetDevice()->GetDeviceInfo()->DeviceId;
				std::map<int,int>::iterator it = m_breakCountMap.find (deviceId);
				if (it != m_breakCountMap.end())
					it->second = 0;

                protocol->UpdateDeiveState(1); //1:在线  0：断线
				if((protocol->m_pChannel->GetChannelState() & 0x01) == 0) {
					protocol->m_pChannel->SetChannelState(1);
				}
			}
			if(isResend)
			{
				m_pResendProtocl = NULL;
			}
		}
	}
	catch(std::exception& e) {
		m_pLogger->LogWarn("[Exception]\r\n%s\r\n[For \"%s\" Channel]", e.what(),
			m_pChannel->GetChannelInfo()->ChannelName);
		if (m_pChannel->IsShowMessage()) m_pChannel->ShowMessage("Engine recv exception!");
		Thread::SLEEP(1000);
	}
	catch (...) {
		m_pLogger->LogWarn("[Exception][For \"%s\" Channel]\r\n\tThe receiving module of the polling engine generates an exception!",
			m_pChannel->GetChannelInfo()->ChannelName);
		if (m_pChannel->IsShowMessage()) m_pChannel->ShowMessage("Engine recv exception!");
		Thread::SLEEP(1000);
	}
}

ST_VOID PollingEngine::OnWorkTask()
{
	if(!m_Working) return;
	try {
	    if(m_pChannel->GetChannelInfo()->ChannelInterval > 0)
	    {
			Thread::SLEEP(this->m_pChannel->GetChannelInfo()->ChannelInterval);
		}
		else {
			Thread::SLEEP(20);
		}
		if(NULL != m_pResendProtocl)
		{
			int maxResendTimes = m_pChannel->GetChannelInfo()->MaxResendTimes;
			int deviceId       = 0;
			int maxBreakTimes  = 0;
			if(NULL != m_pResendProtocl->GetDevice())
			{
				maxResendTimes = m_pResendProtocl->GetDevice()->GetDeviceInfo()->ReSend;
				maxBreakTimes  = m_pResendProtocl->GetDevice()->GetDeviceInfo()->Break;
				deviceId       = m_pResendProtocl->GetDevice()->GetDeviceInfo()->DeviceId;
			}
			if(m_resendCount++ >= maxResendTimes) //当重发次数大于设定的次数就会不再重发
			{
				std::map<int,int>::iterator it = m_breakCountMap.find (deviceId);
				if (it == m_breakCountMap.end())
					m_breakCountMap.insert (make_pair(deviceId, 0));
				else {
					if (++(it->second) >= maxBreakTimes) {
						it->second = 0;
						m_pResendProtocl->UpdateDeiveState(0);//将map数组中记录的重发次数清零，并设置设备为断线状态
					}
				}
				m_pResendProtocl = NULL;
			}
			else
			{
                m_pResendProtocl->ReSend();
				Recieve(m_pResendProtocl, true);
            }
		}
		else if(FindNextDevice())
		{
			if(m_curDevIndex < 0) return;
			ProtocolBase * curProtocol = m_pChannel->GetDevices()->GetItem(m_curDevIndex)->GetProtocol();
			if(NULL == curProtocol && NULL == m_pAppProtocol) return;
			if(curProtocol == NULL)
			{
				curProtocol = m_pAppProtocol;
				curProtocol->SetDevice(m_pChannel->GetDevices()->GetItem(m_curDevIndex));
			}

			if(!m_Working) return;
			if(curProtocol->IsOpened())
			{
				try
				{
                   if(curProtocol->OnSend())
                    {
                        Recieve(curProtocol, false);
                    }
                    if(curProtocol->IsContinue())
                    {
                        this->m_continueIndex = m_curDevIndex;
                    }
				}
				catch(...) {
					if(m_pChannel->IsShowMessage()) {
						curProtocol->ShowMessage("Engine send exception!");
						Thread::SLEEP(2000);
					}
				}
			}
			else
			{
				if(m_pChannel->IsShowMessage()){
					curProtocol->ShowMessage("Port not opened!");
					Thread::SLEEP(2000);
				}
			}
		}
		else
		{
			if(m_pChannel->IsShowMessage()){
				m_pChannel->ShowMessage("No device send data!");
				Thread::SLEEP(2000);
			}
		}
	}
	catch(...) {
		if(m_pChannel->IsShowMessage()) {
			m_pChannel->ShowMessage("Engine work exception!");
			Thread::SLEEP(2000);
		}
	}
}


#ifdef _WIN32
ST_UINT32 PollingEngine::WorkTaskProc(ST_VOID *param)
#else
ST_VOID *PollingEngine::WorkTaskProc(ST_VOID *param)
#endif
{
    PollingEngine *pd=(PollingEngine*)param;
	pd->CheckPort();
    pd->OnWorkTask();
	return 0;
}
