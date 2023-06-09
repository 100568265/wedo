
#include "passthrough.h"

#include "Channel.h"

_PROTOCOL_INTERFACE_IMPLEMENT_(PassThrough)

PassThrough::PassThrough()
{}

PassThrough::~PassThrough()
{}

void PassThrough::Init()
{
}

void PassThrough::Uninit()
{
}

void PassThrough::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
    readed = 0;
	if(!this->GetCurPort() || !this->GetDevice())
		return ;

    Thread::SLEEP(50);
    if (this->GetCurPort()->GetSize() == 0)
        return ;

    Thread::SLEEP(50);

    uint32_t len = min(this->GetCurPort()->GetSize(), 1024);
    readed = this->GetCurPort()->ReadBytes(pbuf, len);
}

ST_BOOLEAN PassThrough::OnSend()
{
	if(this->HasTask() && this->GetTask(&_task))
	{
        if (!strcmp(_task.taskCmd, "transparent_transmission"))
        {
            if (_task.taskParamLen != 0)
                this->Send(_task.taskParam, _task.taskParamLen);
        }
	}
    else
        Thread::SLEEP(50);
}

ST_BOOLEAN PassThrough::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
    int dstDevId = atoi (this->GetDevice()->GetDeviceInfo()->Addressex);
    if (dstDevId <= 0)
        return true;

    ProtocolTask task;
    strcpy(task.taskCmd, "transparent_transmission");
    task.isTransfer     = true;
	task.transChannelId = -1;
    task.transDeviceId  = dstDevId;
    task.taskParamLen   = len;
    task.taskTime       = 1000;
    memcpy (task.taskParam, pbuf, task.taskParamLen);

    Transfer(&task);
}

ST_BOOLEAN PassThrough::IsSupportEngine(ST_INT engineType)
{
	return 0 == engineType; // EngineType == Fulling;
}
