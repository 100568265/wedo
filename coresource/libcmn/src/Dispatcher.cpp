//#include "stdafx.h"
#include "Dispatcher.h"
#include "Communication.h"

Dispatcher::Dispatcher(Communication *communication):
m_Inited (false),
m_Channels(NULL),
m_pCommunication(communication)
{
}

Dispatcher::~Dispatcher()
{

}

ST_VOID Dispatcher::Init()
{
    if(m_Inited) return;
    m_Channels = m_pCommunication->GetChannels();
    m_Inited = true;
}

ST_VOID Dispatcher::Uninit()
{
    if(!m_Inited) return;
    m_Inited = false;
}

ST_BOOLEAN Dispatcher::TransmitTask(ProtocolTask &task)
{
    int oldChannelId = -1, oldDeviceId = -1;
	if(task.transChannelId > 0)
	{   //指定了转发的目标通道
		Channel *transChannel = GetChannel(task.transChannelId);
		if(NULL != transChannel)
		{
			task.isTransfer = false;
            oldChannelId    = task.channelId;
            oldDeviceId     = task.deviceId;
			task.channelId  = task.transChannelId;
			List<Device>* pDevices = transChannel->GetDevices();
			for(int i = 0; i < pDevices->GetCount(); i++)
			{
			    if(task.transDeviceId <= 0)
			    {
			        task.deviceId = pDevices->GetItem(i)->GetDeviceInfo()->DeviceId;
                    task.transChannelId = oldChannelId;
                    task.transDeviceId  = oldDeviceId;
                    transChannel->GetCEngine()->SendTask(task);
                    break; // is continue???
			    }
			    if(pDevices->GetItem(i)->GetDeviceInfo()->DeviceId == task.transDeviceId)
			    {
                    task.deviceId = pDevices->GetItem(i)->GetDeviceInfo()->DeviceId;
                    task.transChannelId = oldChannelId;
                    task.transDeviceId  = oldDeviceId;
                    transChannel->GetCEngine()->OnTask(task);
                    break;
			    }
			}
			return true;
		}
	}
	else
	{          //没有指定转发通道，则具备转发能力的通道都要转发
		if(task.transDeviceId <= 0)
		{
            task.isTransfer     = false;
            task.transChannelId = task.channelId;
            task.transDeviceId  = task.deviceId;
            for(int i = 0; i < m_Channels->GetCount(); i++)
            {
                ChannelInfo *channelinfo = m_Channels->GetItem(i)->GetChannelInfo();
                // int nEnableTransfer = channelinfo->EnableTransfer;
                if(channelinfo->EnableTransfer)
                {
                    Channel *transChannel  = m_Channels->GetItem(i);
                    List<Device>* pDevices = transChannel->GetDevices();
                    if (!pDevices->GetCount())
                        continue;
                    task.channelId  = channelinfo->ChannelID;
                    task.deviceId   = pDevices->GetItem(0)->GetDeviceInfo()->DeviceId;
                    transChannel->GetCEngine()->SendTask(task);
                }
            }
		}
        else
        {
            for(int i = 0; i < m_Channels->GetCount(); i++)
            {
                Channel *transChannel  = m_Channels->GetItem(i);
                List<Device>* pDevices = transChannel->GetDevices();
                for(int j = 0; j < pDevices->GetCount(); j++)
                {
                    if(pDevices->GetItem(j)->GetDeviceInfo()->DeviceId == task.transDeviceId)
                    {
                        task.isTransfer = false;
                        oldChannelId    = task.channelId;
                        oldDeviceId     = task.deviceId;
                        task.channelId  = transChannel->GetChannelInfo()->ChannelID;
                        task.deviceId   = task.transDeviceId;
                        task.transChannelId = oldChannelId;
                        task.transDeviceId  = oldDeviceId;
                        transChannel->GetCEngine()->OnTask(task);
                        return true;
                    }
                }
            }
		}
	}
    return false;
}

Channel *Dispatcher::GetChannel(const ST_INT channelId)
{
    for(int i = 0; i < m_Channels->GetCount(); i++) {
		if(m_Channels->GetItem(i)->GetChannelInfo()->ChannelID == channelId)
            return m_Channels->GetItem(i);
    }
    return NULL;
}
