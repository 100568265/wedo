
#include "zxsf_tempdev.h"
#include "Device.h"
#include "EngineBase.h"


#include "usertask.h"

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

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

Protocol * CreateInstace()
{
	return new zxsf_tempdev();
}

zxsf_tempdev::zxsf_tempdev()
{}

zxsf_tempdev::~zxsf_tempdev()
{}

void zxsf_tempdev::Init ()
{}

void zxsf_tempdev::Uninit()
{}

void zxsf_tempdev::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	if(!this->GetCurPort())
		return;

	int	len = this->GetCurPort()->PickBytes(pbuf, 6, 5000);//拿15个字节出来判断
	if (len < 5)
	{
		this->ShowMessage("Insufficient data length!");
		this->ShowRecvFrame(pbuf,len);
		this->GetCurPort()->Clear();
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
	if (pbuf[2] >= 0x80)
	{
		len = 6;
	}
	else if (pbuf[1] == 0x43) //数据返回
	{
		len = 222;
	}
	else if (pbuf[1] == 0x47) //参数读取返回
	{
		len = 19;
	}
	else if (pbuf[1] == 0x44) //校准返回
	{
		len = 14;
	}
	else if (pbuf[1] == 0x45 || pbuf[1] == 0x46) //存储返回
	{
		len = 7;
	}
	else {
		ShowMessage ("Not Found Function Code!");
		this->GetCurPort()->Clear();
		return;
	}
	int nlen = this->GetCurPort()->PickBytes(pbuf, len, 5000);
	if (nlen == len)
	{
		this->GetCurPort()->ReadBytes(pbuf, len);
		ST_UINT16 wCRC = get_crc16(&pbuf[0], len-2);
		ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
		if(wCRC == nCRC)
		{
			readed = len;
			return;
		}
		else
		{
			this->ShowRecvFrame(pbuf,len);
			ShowMessage ("Check error!");
			this->GetCurPort()->Clear();
			return;
		}
	}
	else
	{
		this->ShowMessage("Insufficient data length!");
		this->ShowRecvFrame(pbuf,len);
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN zxsf_tempdev::OnSend()
{
	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd, "user_by_binary"))
		{
			UserTask1 tasklite;
			memcpy(&tasklite, m_curTask.taskParam, sizeof(tasklite));
			if (!strcmp(tasklite.objName, "UserTask1"))
			{
				if(!strcmp(tasklite.taskCmd,"ReadParam"))
				{
				    SendReadParam(tasklite.taskAddr0);
				}
				else if(!strcmp(tasklite.taskCmd,"SetParam"))
				{
					SendSetParam(tasklite.taskAddr0,tasklite.taskParam,tasklite.taskParamLen); //写入参数
				}
				else if(!strcmp(tasklite.taskCmd,"SetAdjust"))
				{
					SendSetAdjust(tasklite.taskAddr0/*sender地址*/,tasklite.taskValue); //写入参数
				}
				else if(!strcmp(tasklite.taskCmd,"SetAdjust69"))
				{
					SendSetAdjust69(tasklite.taskAddr0/*sender地址*/,tasklite.taskValue*0.1); //存入参数
				}
			}
			else {
				this->TaskResult(-1);
			}
		}
		else {
			this->TaskResult(-1);
		}

		m_bTask = true;
		return true;
	}

	ST_BYTE sendbuf[16] = {0};
	int len;
	sendbuf[0]=(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1]=0x43;
	sendbuf[2]=0x00;
	sendbuf[3]=0x00;
	sendbuf[4]=0x03;
	sendbuf[5]=0x3f;
	sendbuf[6]=0xff;
	sendbuf[7]=0xff;
	sendbuf[8]=0x03;
	len = 9;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
	this->Send(sendbuf,len + 2);

	return true;
}

ST_BOOLEAN zxsf_tempdev::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	if(m_bTask)
	{
		if (pbuf[2] >= 0x80)
		{
			this->ShowRecvFrame(pbuf,len);
			this->TaskResult(-1);
			this->ShowMessage("Return error message.");
			return true;
		}
		if (!strcmp(m_curTask.taskCmd, "user_by_binary"))
		{
			UserTask1 tasklite;
			memcpy(&tasklite, m_curTask.taskParam, sizeof(tasklite));
			if (!strcmp(tasklite.objName, "UserTask1"))
			{
				if(!strcmp(tasklite.taskCmd,"ReadParam") )
				{
					float fvalue;
					for (int i = 0; i < 4; i++)
					{
						fvalue = pbuf[4+i*2]+pbuf[5+i*2]*256;
						this->UpdateValue(500+i,float(fvalue));
					}
					fvalue=short(pbuf[12]+pbuf[13]*256);
					this->UpdateValue(500+4,float(fvalue));
					fvalue=short(pbuf[14]+pbuf[15]*256);
					this->UpdateValue(500+5,float(fvalue));
					fvalue=pbuf[16];
					this->UpdateValue(500+6,float(fvalue));

					this->TaskResult(0);
					return true;
				}
				else if(!strcmp(tasklite.taskCmd,"SetParam"))
				{
					this->TaskResult(0);
					return true;
				}
				else if(!strcmp(tasklite.taskCmd,"SetAdjust"))
				{
					float fvalue=0;
					fvalue=(pbuf[4]+pbuf[5]*256);  //测得的温度值
					this->UpdateValue(0+600,float(fvalue));
					fvalue=(pbuf[6]+pbuf[7]*256);  //未经补偿的标准频偏
					this->UpdateValue(1+600,float(fvalue));
					fvalue=short(pbuf[8]+pbuf[9]*256)*0.1;  //标准频偏误差
					Wvalue=pbuf[8]+pbuf[9]*256;
					this->UpdateValue(2+600,float(fvalue));
					fvalue=(pbuf[10]+pbuf[11]*256);  //信号强度
					this->UpdateValue(3+600,float(fvalue));

					this->TaskResult(0);
					return true;
				}
				else if(!strcmp(tasklite.taskCmd,"SetAdjust69"))
				{
					float fvalue = pbuf[4];  //存储结果
					this->UpdateValue(5 + 600, float(fvalue));
					this->TaskResult(0);
					return true;
				}
			}
		}
	}
	if (pbuf[1]==0x43 && pbuf[3]==0xff && len >10) //解全部数据
	{
		float fvalue = 0;

		for (int i=0;i<108;i++)
		{
			fvalue=pbuf[4+i*2]+pbuf[5+i*2]*256;
			this->UpdateValue(i,float(fvalue));
		}
	}
	return true;
}

ST_BOOLEAN zxsf_tempdev::IsSupportEngine(ST_INT engineType)
{
	return engineType == 1;
}

void zxsf_tempdev::TaskResult(int ret)
{
	m_curTask.taskResult.resultCode = ret;
	m_curTask.isTransfer = 1;
	Transfer(&m_curTask);
	Memset(&m_curTask, 0, sizeof(m_curTask));
}

void zxsf_tempdev::SendReadParam(int Sendreaderaddr)  //读取参数
{
	ST_BYTE sendbuf[16] = {0};
	int len;
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x47;
	sendbuf[2] = 0;
	sendbuf[3] = Sendreaderaddr;
	len = 4;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf, len);
	this->Send(sendbuf, len + 2);
}

void zxsf_tempdev::SendSetParam(int Sendreaderaddr,ST_BYTE * buf,ST_BYTE len)//写入参数
{
	ST_BYTE sendbuf[24] = {0};
	// len;
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x46;
	sendbuf[2] = 00;
	sendbuf[3] = Sendreaderaddr;
	for (int i = 4; i < 15; ++i)
	{
		sendbuf[i] = buf[i-4];
	}
	len = 15;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
	this->Send(sendbuf,len+2);
}

void zxsf_tempdev::SendSetAdjust(int Sendreaderaddr,ST_UINT16 fvalue)
{

	ST_BYTE sendbuf[16] = {0};
	int  len;
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x44;
	sendbuf[2] = 00;
	sendbuf[3] = Sendreaderaddr;
	sendbuf[4] = fvalue % 256;
	sendbuf[5] = fvalue / 256;
	len = 6;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
	this->Send(sendbuf,len+2);
}

void zxsf_tempdev::SendSetAdjust69(int Sendreaderaddr,float fvalue)
{

	ST_BYTE sendbuf[16] = {0};
	int  len;

	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x45;
	sendbuf[2] = 00;
	sendbuf[3] = Sendreaderaddr;
	sendbuf[4] = Wvalue % 256;
	sendbuf[5] = Wvalue / 256;
	len = 6;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
	this->Send(sendbuf, len + 2);
}
