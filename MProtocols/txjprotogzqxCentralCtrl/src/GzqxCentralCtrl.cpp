
#include "GzqxCentralCtrl.h"

#include "Channel.h"

_PROTOCOL_INTERFACE_IMPLEMENT_(GzqxCentralCtrl)

#pragma pack(push,1)	// 紧凑对齐

inline uint8_t get_check_sum (const void * data, int len)
{
    uint8_t sum = 0;
    const uint8_t * ptr = (uint8_t*) data;
    while (len --> 0)
        sum += *ptr++;

    return sum;
}

struct frame_header
{
    const static uint8_t start_value = 'w';

    enum func_enum {
        FC_PASS_THROUGH_ASK = 0xA3,
        FC_PASS_THROUGH_CON = 0xC3,
        FC_HEARTBEAT_ASK    = 0x43,
        FC_HEARTBEAT_CON    = 0x83
    };

    uint8_t  start;
    uint8_t  chk;
    uint8_t  func;
    uint8_t  len;
    uint16_t devid;
};

#pragma pack(pop)		// 恢复默认内存对齐

GzqxCentralCtrl::GzqxCentralCtrl()
{}

GzqxCentralCtrl::~GzqxCentralCtrl()
{}

void GzqxCentralCtrl::Init()
{
}

void GzqxCentralCtrl::Uninit()
{
}

void GzqxCentralCtrl::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
    readed = 0;
	if(!this->GetCurPort() || !this->GetDevice())
		return ;

    uint32_t len = this->GetCurPort()->PickBytes(pbuf, sizeof(frame_header), 1000);
    if (len < sizeof(frame_header))
        return ;

    uint32_t index = 0;
    for (; index < len; ++index)
    {
        if (pbuf[index] == frame_header::start_value)
            break;
    }

	if(index == len) {
		ShowMessage ("Garbled code, clear buffer");
		this->GetCurPort()->ReadBytes(pbuf, index);
		this->GetCurPort()->Clear();
		return ;
	}
	if(index > 0) {
		ShowMessage ("Has Garbled code");
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, index);
	}

    len = this->GetCurPort()->PickBytes(pbuf, sizeof(frame_header), 1000);
    if (len < sizeof(frame_header)) {
        ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return ;
    }

    frame_header & hdr = *(frame_header*)pbuf;
    uint32_t framelen = hdr.len + sizeof(frame_header);
    len = this->GetCurPort()->PickBytes(pbuf, framelen, 1000);
    if (len < framelen) {
        ShowMessage ("Insufficient data length.");
		this->GetCurPort()->ReadBytes(pbuf, len);
		return ;
    }

    if (get_check_sum(&(hdr.func), framelen - 2) != hdr.chk)
    {
        ShowMessage ("Check error!");
        this->GetCurPort()->ReadBytes(pbuf, framelen);
        return ;
    }

    readed = this->GetCurPort()->ReadBytes(pbuf, framelen);
}

ST_BOOLEAN GzqxCentralCtrl::OnSend()
{
	if(this->HasTask() && this->GetTask(&_task))
	{
        if (!strcmp(_task.taskCmd, "transparent_transmission"))
        {
            if (_task.taskParamLen > 0) {
                std::vector<uint8_t> data;
                data.resize(sizeof(frame_header));
                data.insert (data.end(), _task.taskParam, _task.taskParam + _task.taskParamLen);

                frame_header & hdr = *(frame_header*)&data[0];

                hdr.start = frame_header::start_value;
                hdr.func  = frame_header::FC_PASS_THROUGH_CON;
                hdr.devid = _task.transDeviceId;
                hdr.len   = _task.taskParamLen;
                hdr.chk   = get_check_sum (&(hdr.func), hdr.len + sizeof(frame_header) - 2);

                this->Send (&data[0], data.size());
            }
        }
	}
    else
        Thread::SLEEP(50);

    return true;
}

ST_BOOLEAN GzqxCentralCtrl::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
    frame_header & hdr = *(frame_header*)pbuf;
    switch (hdr.func) {
        case frame_header::FC_PASS_THROUGH_ASK: {
            ProtocolTask task;
            strcpy(task.taskCmd, "transparent_transmission");
            task.isTransfer     = true;
            task.transChannelId = -1;
            task.transDeviceId  = hdr.devid;
            task.taskParamLen   = hdr.len;
            task.taskTime       = 1000;
            memcpy (task.taskParam, pbuf + sizeof(frame_header), task.taskParamLen);
            Transfer(&task);

            if(hdr.devid == 4)
                tranferNewTask(pbuf[11]);
        } break;
        case frame_header::FC_HEARTBEAT_ASK: {
            uint8_t data[8] = {0};
            frame_header & hdr = *(frame_header*)&data[0];

            hdr.start = frame_header::start_value;
            hdr.func  = frame_header::FC_HEARTBEAT_CON;
            hdr.devid = 0;
            hdr.len   = 0;
            hdr.chk   = get_check_sum (&(hdr.func), hdr.len + sizeof(frame_header) - 2);

            this->Send (&data[0], 6);
        } break;
        case frame_header::FC_HEARTBEAT_CON: {

        } break;
    }

    return true;
}

ST_BOOLEAN GzqxCentralCtrl::IsSupportEngine(ST_INT engineType)
{
	return 0 == engineType; // EngineType == Fulling;
}

void    GzqxCentralCtrl::tranferNewTask(bool bIsOn)
{
    ProtocolTask task;
    strcpy(task.taskCmd, "transparent_transmission");
    task.isTransfer     = true;
    task.transChannelId = -1;
    task.transDeviceId  = 8;  //new device
    task.taskParamLen   = 5;
    task.taskTime       = 1000;
    //memcpy (task.taskParam, pbuf + sizeof(frame_header), task.taskParamLen);
    task.taskParam[0] = 0xD5;
    task.taskParam[1] = 0x11;
    task.taskParam[2] = 0x10;
    task.taskParam[3] = bIsOn?0x01:0x00;
    task.taskParam[4] = 0xAA;

    Transfer(&task);
}
