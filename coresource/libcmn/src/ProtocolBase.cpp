//#include "stdafx.h"
#include "EngineBase.h"
#include "ProtocolBase.h"
#include "Channel.h"
#include "Device.h"
#include "Devices.h"
#include "syslogger.h"

#include <stdio.h>

ProtocolBase::ProtocolBase()
{
	m_pChannel      = NULL;
	m_pDevice       = NULL;
	m_pPorts        = NULL;
	m_BakBuffer     = NULL;
	m_BakSendLength = 0;
	m_MaxSendLength = 0;
    m_pLogger       = SysLogger::GetInstance();
}

ProtocolBase::~ProtocolBase()
{
	if(NULL != m_BakBuffer) {
		delete[] m_BakBuffer;
		m_BakBuffer = NULL;
	}
}

ST_VOID ProtocolBase::Init()
{
}

ST_VOID ProtocolBase::Uninit()
{
}

ST_VOID ProtocolBase::Start()
{
	if(!this->IsOpened()) {
        this->Open();
    }
}

ST_VOID ProtocolBase::Stop()
{
}

ST_VOID ProtocolBase::Open()
{
	if(m_pPorts) {
		if (m_pPorts->GetChannel()->GetChannelInfo()->AutoOpen != 1)//linweiming
		{
			m_pPorts->Open();
		}
	}
}

ST_VOID ProtocolBase::Close()
{
	if(m_pPorts) {
		if (m_pPorts->GetChannel()->GetChannelInfo()->AutoOpen !=1)//linweiming
		{
			m_pPorts->Close();
		}
	}
}

ST_VOID ProtocolBase::Close(ST_UINT64 port)
{
	if (m_pPorts)
		m_pPorts->Close(port);
}

ST_BOOLEAN ProtocolBase::IsOpened()
{
	return (m_pPorts ? m_pPorts->IsOpened() : false);
}

ST_BOOLEAN	ProtocolBase::IsContinue()
{
	ST_BOOLEAN c = this->m_continue;
	m_continue = false;
	return c;
}

ST_VOID	ProtocolBase::Continue()
{
	m_continue = true;
}

ST_VOID	ProtocolBase::OnConnect(ST_INT port,ST_UINT64 portAddr)
{

}

ST_VOID	ProtocolBase::OnDisconnect(ST_INT port,ST_UINT64 portAddr)
{

}

ST_BOOLEAN	ProtocolBase::OnProcess(ST_BYTE *pBuf, ST_INT len)
{
	return true;
}

ST_BOOLEAN  ProtocolBase::OnRead(PortTask *task)
{
	return false;
}

ST_BOOLEAN ProtocolBase::Send(ST_BYTE *pBuf,ST_INT len,ST_UINT64 dstAddr)
{
	if(NULL != m_pPorts) {
		if(m_pPorts->IsOpened()) {
			if(len > m_MaxSendLength) {
				m_MaxSendLength = len;
				if (m_BakBuffer) delete[] m_BakBuffer;
				m_BakBuffer = new ST_BYTE[m_MaxSendLength];
			}
			m_DestAddr = dstAddr;
			Memmove (m_BakBuffer, pBuf, len);
			m_BakSendLength = len;
			ST_BOOLEAN ret = m_pPorts->Send(pBuf, len, dstAddr);
			//m_pLogger->LogDebug("pBuf = %d, len = %d, dstAddr = %d",pBuf,len,dstAddr);
			if(ret) {
				if(m_pChannel->IsShowMessage()) {
					ShowSendFrame(pBuf,len);
				}
			}
			else{
				ShowSendFrame(pBuf, len);
				sprintf(m_Msg, "Send to device addr %d failed!",dstAddr);
				ShowMessage(m_Msg);
			}
			return ret;
		}
		else{
			ShowMessage("Port not opened, cancel send data!");
			Thread::SLEEP(2000);
		}
	}
	// else
	// {
	// 	   OnPortBreak();
	// }
	return false;
}





ST_BOOLEAN  ProtocolBase::Transfer(ProtocolTask * task)
{
    task->isTransfer = true;
    task->channelId  = m_pChannel->GetChannelInfo()->ChannelID;
    task->deviceId   = m_pDevice->GetDeviceInfo()->DeviceId;
    // task->transChannelId=task->transChannelId;
    // task->transDeviceId=task->transDeviceId;
    if(task->channelId == task->transChannelId) return false;
//    m_pLogger->LogDebug(" Transfer   task->taskValue = %f",task->taskValue);
	m_pChannel->GetCEngine()->SendTask(*task);
	return true;
}

ST_BOOLEAN	ProtocolBase::ReSend()
{
	if(m_BakSendLength > 0) {
		ShowMessage("Link resend!");
		return Send(m_BakBuffer, m_BakSendLength, -1);
	}
	return false;
}

DataCache *ProtocolBase::GetCurPort(ST_UINT64 port)
{
	if ( ! m_pChannel)
		return NULL;
	if ( ! m_pChannel->GetCEngine()->m_pPortBuffer)
		return NULL;
	return m_pChannel->GetCEngine()->m_pPortBuffer->GetCache(port);
}

ST_BOOLEAN ProtocolBase::HasTask()
{
    return !m_ProtocolTasks.IsEmpty();
}

ST_BOOLEAN ProtocolBase::GetTask(ProtocolTask *task, ST_BOOLEAN pop)
{
	if (NULL == task) return false;

    TaskNode* node = pop ?
    		m_ProtocolTasks.Pop():
    		m_ProtocolTasks.Peek();

	if(NULL == node)
		return false;

	*task = *(node->obj);
	/*if(task->taskList)
        delete task->taskList;
    Memcpy(task, node->pbuf, node->len);

	if(node->pNext != NULL) {
		task->taskList = new vector<ST_BYTE>();
		for(int i = 0; i < node->pNext->len; i++) {
			task->taskList->push_back(node->pNext->pbuf[i]);
		}
		delete node->pNext;
	}*/
	if (pop) {
		delete node;
		node = NULL;
	}
	return true;
}

ST_VOID ProtocolBase::OnTask(ProtocolTask *task)
{
	if (!task) return;

    TaskNode* node = new TaskNode();
//    node->obj = wedo::shared_ptr<ProtocolTask>(new ProtocolTask(*task));
    node->obj.reset (new ProtocolTask(*task));
/*	node->len = sizeof(ProtocolTask);
	node->pbuf = new ST_BYTE[node->len];
	node->pNext=NULL;
	Memcpy(node->pbuf,task,node->len);

	if(task->taskList->size()>0){
		ST_BYTE *addr=&(*task->taskList)[0];
		TaskNode* next = new TaskNode();
		next->len = task->taskList->size();
		next->pbuf = new ST_BYTE[next->len];
		Memcpy(next->pbuf, addr, next->len);
		node->pNext=next;
	}*/
	m_ProtocolTasks.Push(node);
}

ST_VOID	ProtocolBase::OnTask(ProtocolTask *task, ST_BYTE *data, ST_INT dataLen)
{
	if (!data || dataLen <= 0)
		return;
	if (! task->taskList)
		task->taskList = new std::vector<ST_BYTE>();
	task->taskList->insert (task->taskList->end(), data, data + dataLen);
	/*for(int i = 0; i < dataLen; i++) {
		task->taskList->push_back(data[i]);
	}*/
	OnTask(task);
}

ST_VOID ProtocolBase::OnTaskBack(ProtocolTask *task)
{
    if (m_pDevice && m_pDevice->GetEngine())
        m_pDevice->GetEngine()->OnTaskBack(*task);
}

ST_VOID ProtocolBase::SetDevice(Device *device)
{
    m_pDevice = device;
}

Device *ProtocolBase::GetDevice()
{
	return m_pDevice;
}

Devices *ProtocolBase::GetDevices()
{
	return (m_pPorts ? m_pPorts->GetChannel()->GetCEngine()->GetDevices() : NULL);
}

PortBase *ProtocolBase::GetMainPort()
{
	return (m_pPorts ? m_pPorts->GetMainPort() : NULL);
}

PortBase *ProtocolBase::GetSlavPort()
{
	return (m_pPorts ? m_pPorts->GetSlavPort() : NULL);
}

ST_INT ProtocolBase::GetDeviceId()
{
	return (m_pDevice? m_pDevice->GetDeviceInfo()->DeviceId : -1);
}

ST_VOID ProtocolBase::OnLinkBreak()
{
	if(m_bBreak) return;
	m_bBreak = true;
//	this->ShowMessage("通讯中断");
//	snprintf(m_Msg, sizeof(m_Msg), "数据采集通道[%s]  %s 通讯中断",
//	this->m_pChannel->GetChannelInfo()->ChannelName, this->m_pDevice->GetDeviceInfo()->DeviceName);
	UpdateDeiveState(0);
}

ST_VOID	ProtocolBase::UpdateDeiveState(ST_INT state)
{
	ST_CHAR varName[296] = {0};
	snprintf(varName, sizeof(varName), "devicestate.%s.value",
				this->m_pDevice->GetDeviceInfo()->DeviceName);
	ST_VARIANT var;
	var.vt  = VALType_UInt32;
	var.iVal= state;
	SetVariableValue(varName,var);
}

ST_VOID ProtocolBase::ShowMessage(const ST_CHAR *msg)
{
	m_pChannel->ShowMessage(msg, GetDeviceId());
}

ST_VOID ProtocolBase::ShowSendFrame(ST_BYTE *buf, ST_INT len)
{
	m_pChannel->ShowSendFrame(buf, len, GetDeviceId());
}

ST_VOID ProtocolBase::ShowRecvFrame(ST_BYTE *buf, ST_INT len)
{
	m_pChannel->ShowRecvFrame(buf, len, GetDeviceId());
}

SysLogger *ProtocolBase::GetLogger()
{
	return m_pLogger;
}
