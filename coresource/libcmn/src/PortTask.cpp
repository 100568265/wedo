
#include "PortTask.h"

#include "sysmalloc.h"

PortTask::PortTask()
{
    DeviceAddr=-1;
    PortSrcAddr=-1;
    PortDstAddr=-1;
    LocalChannelID=-1;
    TransmitChannelId=-1;
    IsTransfer=false;
	KnowIPAddr=false;
}

PortTask::PortTask(const PortTask &task)
{
    DeviceAddr=task.DeviceAddr;
    PortSrcAddr=task.PortSrcAddr;
    PortDstAddr=task.PortDstAddr;
    LocalChannelID=task.LocalChannelID;
    TransmitChannelId=task.TransmitChannelId;
    IsTransfer=task.IsTransfer;
	KnowIPAddr=task.KnowIPAddr;
    Memcpy(m_Buffer,task.m_Buffer,task.m_curLength);
    m_curLength=task.m_curLength;
}

PortTask::~PortTask()
{

}

ST_BYTE *PortTask::GetBuffer()
{
    return m_Buffer;
}

ST_INT PortTask::GetBufferLen()
{
    return m_curLength;
}

ST_VOID PortTask::Write(ST_BYTE *buf,ST_INT size)
{
	m_curLength=size;
    if(size>bufLen || size<=0) return;
    Memcpy(m_Buffer,buf,size);
    m_curLength=size;
}


