#include "PMC.h"
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


uint16_t CPMC::GetCRC16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}


CPMC::CPMC()
{
    //ctor
}

CPMC::~CPMC()
{
    //dtor
}

void CPMC::Init()
{

}


void CPMC::Uninit()
{

}


void CPMC::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
	// 第一个字节地址域 范围1-247
	if (this->GetCurPort())
	{
		m_portbreak = false;

		int lineinterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		if (lineinterval < 3000)
			lineinterval = 3000;

		int len = this->GetCurPort()->PickBytes(pbuf,4,lineinterval); // 地址域 功能码 CRC
		if (len >= 4)
		{
			int star = 0;
			for (; star<len; star++)
			{
				if (pbuf[star] == (ST_BYTE) this->GetDevice()->GetDeviceInfo()->Address)
					break;
			}
			if (star == len)
			{
				this->ShowMessage("Garbled code, clear buffer.");
				this->ShowRecvFrame(pbuf,len);
				this->GetCurPort()->Clear();
				return;
			}
			if (star > 0)
				this->GetCurPort()->ReadBytes(pbuf,star);

			len = this->GetCurPort()->PickBytes(pbuf,4,lineinterval);
			if (len < 4)
			{
				//this->ShowMessage("报文长度不足,抛弃该报文.",0);
				//this->ShowRecvFrame(pbuf,len);
				this->GetCurPort()->Clear();
				return;
			}

			// 分情况读取了
			if (pbuf[2] & 0x80) // 子站异常应答
			{  // 异常报文结构：子站地址、响应帧总长度、功能码、故障码和校验域
				this->ShowMessage("Device error response.");
				switch(pbuf[3])
				{
				case 0x01: this->ShowMessage("The reason:device not support this function code."); break;
				case 0x02: this->ShowMessage("The reason:device not support this register addr."); break;
				case 0x03: this->ShowMessage("The reason:request register number too much."); break;
				default:break;
				}
				this->GetCurPort()->Clear();
				return;
			}
			else
			{ // 正常情况
				// 结构：从站地址，响应帧总长度,功能码
				len = pbuf[1];
				int nlen = this->GetCurPort()->ReadBytes(pbuf,len,lineinterval);
				if (nlen >= len)
				{
					ST_UINT16 wCrc = GetCRC16(pbuf,len-2);
					ST_UINT16 rCrc = *((ST_UINT16*)&pbuf[len-2]); // 自动将高低位换过来 不同系统可能会不同
					if (wCrc == rCrc)  // ???
						readed = nlen;
					else
					{
						this->ShowMessage("CRCCheck error.");
						this->ShowRecvFrame(pbuf,nlen);
					}
				}
				else
				{
					this->ShowMessage("Insufficient data length.");
					this->GetCurPort()->Clear();
				}
			}
		}
		else
		{ // 长度不足
			this->ShowMessage("Insufficient data length.");
			this->GetCurPort()->Clear();
		}
	}
	else if (!m_portbreak)
	{
		m_portbreak = true;
		this->OnLinkBreak();
	}
}


ST_BOOLEAN	CPMC::OnSend()
{
    AskRTValue(0xAA);
    Thread::SLEEP(50);
	return true;
}


ST_BOOLEAN	CPMC::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    switch(pbuf[2])
	{
	case 0x25:
		{
			// 默认只读取一种吧。 如果多种都要读
			switch(m_curReadIndex)
			{
			case 0: // 发送0xAA
				{ // 40000 - 40018 pbuf[4] 开始
					soeptr = GetDWORD(&pbuf[4]);		 // 4 5 6  7
					uint16_t stateh = pbuf[8]*256 + pbuf[9]; // 8 9 10 11
					uint16_t statel = pbuf[10]*256+ pbuf[11];

					uint8_t data = (statel & 0x0001)? 1:0;
					this->UpdateValue(0,data);

					data = (statel & 0x0002)? 1:0;
					this->UpdateValue(1,data);

					data = (statel & 0x0004)? 1:0;
					this->UpdateValue(2,data);

					data = (statel & 0x0010)? 1:0;
					this->UpdateValue(3,data);

					data = (statel & 0x0020)? 1:0;
					this->UpdateValue(4,data);

					data = (statel & 0x0040)? 1:0;
					this->UpdateValue(5,data);

					data = (statel & 0x0100)? 1:0;
					this->UpdateValue(6,data);

					data = (statel & 0x0200)? 1:0;
					this->UpdateValue(7,data);

					data = (statel & 0x0400)? 1:0;
					this->UpdateValue(8,data);


					data = (stateh & 0x0001)? 1:0;
					this->UpdateValue(9,data);

					data = (stateh & 0x0002)? 1:0;
					this->UpdateValue(10,data);

					data = (stateh & 0x0004)? 1:0;
					this->UpdateValue(11,data);

					data = (stateh & 0x0010)? 1:0;
					this->UpdateValue(12,data);

					data = (stateh & 0x0020)? 1:0;
					this->UpdateValue(13,data);

					data = (stateh & 0x0040)? 1:0;
					this->UpdateValue(14,data);

					data = (stateh & 0x0100)? 1:0;
					this->UpdateValue(15,data);

					data = (stateh & 0x0200)? 1:0;
					this->UpdateValue(16,data);

					data = (stateh & 0x0400)? 1:0;
					this->UpdateValue(17,data);


					// 12 13
					// 14 15
                    uint16_t di = pbuf[16]*256 + pbuf[17];
					data = (di & 0x0001)? 1:0;
					this->UpdateValue(18,data);

					data = (di & 0x0002)? 1:0;
					this->UpdateValue(19,data);


					uint16_t _do = pbuf[18]*256 + pbuf[19];
					data = (_do & 0x0001)? 1:0;
					this->UpdateValue(20,data);

					data = (_do & 0x0002)? 1:0;
					this->UpdateValue(21,data);

					data = (_do & 0x0100)? 1:0;
					this->UpdateValue(22,data);
					// 20 21

					int index = 0;
					for (;index<this->GetDevice()->GetDeviceInfo()->DataAreasCount; index++)
					{

						if (this->GetDevice()->GetDeviceInfo()->DataAreas[index].areaName == "YC")
							break;
					}
					if (index == this->GetDevice()->GetDeviceInfo()->DataAreasCount)
					{
						this->ShowMessage("No find YC DataAreas.");
					}
					else
						for (int i=0; i<5; i++)
						{
							ST_UINT32 value = GetDWORD(&pbuf[22+i*4]); // 22 23 24 25
							const DataAreaItem &itemref =this->GetDevice()->GetDeviceInfo()->DataAreas[index].items[i];
							this->UpdateValue(10000+i,value*itemref.coeficient);
						}
				}break;
			case 1:
				{

				}break;
			default: break;
			}
		}break;
	default: break;
	}
	return true;
}


ST_BOOLEAN	CPMC::IsSupportEngine(ST_INT engineType)
{
    return true;
}

void CPMC::AskSoe(ST_UINT32 ptr)
{
    ST_BYTE sendbuf[32] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x24;
	ST_UINT16 buf = ptr >> 16;
	sendbuf[2] = buf / 256;
	sendbuf[3] = buf % 256;
	buf = ptr & 0x0000FFFF;
	sendbuf[4] = buf / 256;
	sendbuf[5] = buf % 256;
	ST_UINT16 crc = GetCRC16(sendbuf,6);
	sendbuf[6] = crc % 256;
	sendbuf[7] = crc / 256;
	this->Send(sendbuf,8);
}

void CPMC::AskRTValue(ST_INT type)
{
    ST_BYTE sendbuf[32] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x25;
	sendbuf[2] = (ST_BYTE)type;
	ST_UINT16 crc = GetCRC16(sendbuf,3);
	sendbuf[3] = crc % 256;
	sendbuf[4] = crc / 256;
	this->Send(sendbuf,5);
}


void CPMC::SetParam(ST_BYTE typ, ST_UINT16 addr, ST_BYTE *datas)
{
    ST_BYTE sendbuf[256] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = typ; // 0x26 是系统参数 0x27 是保护参数
	sendbuf[2] = addr / 256;
	sendbuf[3] = addr % 256;
	sendbuf[4] = datas[0];
	sendbuf[5] = datas[1];
	memcmp(&sendbuf[6],&datas[2],sendbuf[5]);
	int len = sendbuf[5] + 6;
	ST_UINT16 crc = GetCRC16(sendbuf,len);
	sendbuf[len] = crc % 256;
	sendbuf[len+1] = crc / 256;
	this->Send(sendbuf,len+2);
}


ST_UINT32 CPMC::GetDWORD(ST_BYTE *buf)
{
    ST_UINT32 value = 0;
	value = buf[0];
	value = value << 8;
	value += buf[1];
	value = value << 8;
	value += buf[2];
	value = value << 8;
	value += buf[3];
	return value;
}










