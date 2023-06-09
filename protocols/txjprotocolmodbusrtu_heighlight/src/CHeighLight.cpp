#include "CHeighLight.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

static const uint16_t crc16_table[256] =
{
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
	0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
	0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
	0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
	0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
	0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
	0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
	0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
	0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
	0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
	0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
	0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
	0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
	0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
	0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
	0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
	0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
	0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
	0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
	0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
	0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
	0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
	0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
	0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
	0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
	0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
	0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
	0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};


CHeighLight::CHeighLight()
{
    //ctor
}

CHeighLight::~CHeighLight()
{
    //dtor
}

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

class convert
{
public:

	template<typename T>
	static T bytes_to (const void * bytes, size_t len)
	{
		T value;
		memcpy (&value, bytes, min(sizeof(T), len));
		return value;
	}

};

CHeighLight* CreateInstace()
{
    return new CHeighLight();
}

ST_BOOLEAN	CHeighLight::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	CHeighLight::Init()
{
	m_bTask = false;
	m_curreadIndex = 0;
	m_readIndex = 0;
}

void	CHeighLight::Uninit()
{

}
void	CHeighLight::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	if(! this->GetCurPort())
		return;
	if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
	{
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	if(len < 5) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	for(; star < len; ++star) {
		if(pbuf[star] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
				break;
	}
	if(star > 0) {
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	if(star == len) {
		//全是乱码
		ShowMessage ("Garbled code, clear buffer.");
		this->GetCurPort()->Clear();
		return;
	}
	len = this->GetCurPort()->PickBytes(pbuf, 5, 2000);
	ST_BYTE fuccode = pbuf[1 + star];
	if((fuccode & 0xF0) == 0x80)
	{
		len = 5;
	}
	else if(fuccode == 0x05)
	{
		len = 8;
	}
	else if((fuccode == 0x06) && m_bTask)
	{
		len = m_curTask.taskParamLen + 6;
	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
	{
		//读数据返回
		ST_BYTE readCount = pbuf[2 + star];
		len = readCount + 5;
	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].writeCode)
	{
		//写数据返回
		len = 8;
	}
	else
	{
		ShowMessage ("Not Found Function Code!");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 2000);
	if(nlen == len)
	{
		this->GetCurPort()->ReadBytes(pbuf, len);
		ST_UINT16 wCRC = get_crc16(&pbuf[0], len-2);
		ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
		{
			readed = len;
			return;
		}
	}
	else
	{
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN	CHeighLight::OnSend()
{
	const DeviceInfo* info = this->GetDevice()->GetDeviceInfo();
	if(info && info->DataAreasCount > 0)
	{
		if (m_readIndex >= info->DataAreasCount)
			m_readIndex = 0;
		m_curreadIndex = m_readIndex++;
		SendReadCmd(info->DataAreas[m_curreadIndex].readCode, (ST_UINT16)info->DataAreas[m_curreadIndex].addr,
			       info->DataAreas[m_curreadIndex].dataUnitLen > 1 ?
			       info->DataAreas[m_curreadIndex].len / info->DataAreas[m_curreadIndex].dataUnitLen : info->DataAreas[m_curreadIndex].len);
	}
//	ShowMessage("enter function");
	return true;
}

ST_BOOLEAN	CHeighLight::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
	if (pbuf[1]==0x02)
	{
		ST_BYTE by = 0x01;
		for (ST_INT j=0;j<8;j++)
		{
			ST_BYTE byvalue = 0x00;
			//	byvalue = pbuf[4+i]&(by<<j);
			byvalue =(pbuf[4]>>j)&by;
			this->UpdateValue(j,(ST_BYTE)byvalue?1:0); //,&pbuf[4],1
			if(m_Statu[j] != byvalue)
			{
				m_Statu[j] = (ST_BYTE)byvalue?1:0;
				ProtocolTask *task 		= new ProtocolTask;
                task->isTransfer 		= true;
                task->transChannelId 	= -1;
                task->transDeviceId 	= 20;
                task->taskAddr 			= j;
                task->taskValue 		= byvalue;
                task->taskCmdCode       = 1;
                task->timeOut           = 2000;
	            strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));
                Transfer(task);
                delete task;
			}//file:///home/work/wedo/comm/protocols/txjprotocolmodbusrtu_heighlight/bin/Debug/libtxjprotocolmodbusrtu_heighlight.so

		}
		for (ST_INT k=0;k<8;k++)
		{
			ST_BYTE byvalue = 0x00;
			//	byvalue = pbuf[3+i]&(by<<k);
			byvalue =(pbuf[3]>>k)&by;
			this->UpdateValue(8+k,(ST_BYTE)byvalue?1:0);//,&pbuf[3],1
			if(m_Statu[8+k] != byvalue)
			{
				m_Statu[8+k] = (ST_BYTE)byvalue?1:0;
                ProtocolTask *task 		= new ProtocolTask;
                task->isTransfer 		= true;
                task->transChannelId 	= -1;
                task->transDeviceId 	= 21;
                task->taskAddr 			= k;
                task->taskValue 		= byvalue;
                task->taskCmdCode       = 1;
                task->timeOut           = 2000;
	            strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));
                Transfer(task);
                delete task;
			}
		}
	}
	return true;
}

void CHeighLight::SendReadCmd(ST_BYTE code,ST_UINT readAddr,ST_UINT count)
{
	ST_BYTE sendbuf[8];
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = code;
	FillWORD(sendbuf + 2,readAddr);
	FillWORD(sendbuf + 4,count);
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);
}

void CHeighLight::FillWORD(ST_BYTE* buf,ST_UINT v)
{
	ST_BYTE* pv = (ST_BYTE*)&v;
	buf[0] = pv[1];
	buf[1] = pv[0];
}
