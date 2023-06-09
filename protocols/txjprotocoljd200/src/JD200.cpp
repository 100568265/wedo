// JD200.cpp: implementation of the CJD200 class.
//
//////////////////////////////////////////////////////////////////////

#include "JD200.h"
#include "Device.h"
#include "Channel.h"
#include "datetime.h"
#include <math.h>

static const uint16_t crc16_table[256] = {
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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const char * soestr[] = {
	"速断动作",
	"限时速断动作",
	"定时过流动作",
	"零序过流保护动作",
	"充电保护动作",
	"过负荷保护动作",
	"重合闸动作",
	"低周减载动作",
	"后加速保护动作",
	"控制回路断线告警",
	"开出回路故障告警",
	"模入回路故障告警",
	"母线零序过压告警",
	"过压保护动作",
	"母线过压保护告警",
	"接地告警",
	"母线PT断线告警",
	"复合电压闭锁过流I段保护动作",
	"复合电压闭锁过流II段保护动作",
	"复合电压闭锁过流III段保护动作",
	"零序过压保护动作",
	"欠压保护动作",
	"过负荷保护告警",
	"备自投联切负荷动作",
	"定值出错",
	"定值区出错",
	"跳进线Ⅰ开关拒动",
	"手动合闸",
	"手动跳闸",
	"RAM出错",
	"ROM出错",
	"频率出错",
	"装置地址出错",
	"电度系数出错",
	"断路器处于分闸位置",
	"时钟出错",
	"模拟量系数出错",
	"轻瓦斯告警",
	"隔离开关投入",
	"重瓦斯跳闸",
	"弹簧储能足",
	"断路器处于合闸位置",
	"跳进线Ⅱ开关拒动",
	"不对应重合闸",
	"接地刀闸投入",
	"装置通讯出错",
	"装置通讯恢复",
	"过温保护告警",
	"超温保护跳闸",
	"遥控合闸",
	"遥控分闸",
	"反时限过流保护动作",
	"启动时间过长保护动作",
	"不平衡电流保护动作",
	"不平衡电压保护动作",
	"母线欠压保护告警",
	"PT切换动作",
	"PT自动解列",
	"Ⅰ段母线过压保护告警",
	"Ⅱ段母线过压保护告警",
	"Ⅰ段母线欠压保护告警",
	"Ⅱ段母线欠压保护告警",
	"Ⅰ段母线零序电压保护告警",
	"Ⅱ段母线零序电压保护告警",
	"Ⅰ段母线PT断线告警",
	"Ⅱ段母线PT断线告警",
	"对侧无电压",
	"合进线Ⅰ开关拒动",
	"合进线Ⅱ开关拒动",
	"合母联开关拒动",
	"对侧电压异常",
	"跳母联开关拒动",
	"进线Ⅰ自复位成功",
	"进线Ⅱ自复位成功"
};

Protocol * CreateInstace()
{
	return new CJD200();
}

CJD200::CJD200()
{
	m_bTask        = false;
	m_curreadIndex = 0;
	m_readIndex    = 0;
	newminute      = 0;
	oldminute      = 0;
	timecount      = 0;
	readindex      = 0;
	iscleansoeflag = 0x00;
	m_tmSend       = time(0);
	m_nWaitTime    = 0;

	m_bbtncontrol  = 0;
	m_nykpoint     = 0x00;
	m_nison        = 0x00;
}

CJD200::~CJD200()
{
}

void CJD200::Init()
{
}

void CJD200::Uninit()
{
}

bool CJD200::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void CJD200::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;

	if(this->GetCurPort())
	{
		// int ntimeout = atoi(this->m_pChannel->m_channelParam.channeladdr);
		// if (ntimeout <= 0) ntimeout = 1000;
		if (this->GetDevice() && this->GetDevice()->GetDeviceInfo()->ReSend > 0)
		{
			this->GetDevice()->GetDeviceInfo()->Break *= this->GetDevice()->GetDeviceInfo()->ReSend;
			this->GetDevice()->GetDeviceInfo()->ReSend = 0;
		}
		int	len = this->GetCurPort()->PickBytes(pbuf, 5, 1000 + m_nWaitTime);
		if(len >= 5)
		{
			int star = 0;
			for(; star < len; ++star)
				if(pbuf[star] == 0x5A)
					break;

			if(star > 0) {
				//star大于0，说明有乱码， 把之前的乱码丢掉
				this->GetCurPort()->ReadBytes(pbuf, star);
			}
			if(star == len) {
				//全是乱码
				this->ShowRecvFrame(pbuf, len);
				this->GetCurPort()->Clear();
				return;
			}
			this->GetCurPort()->PickBytes(pbuf, 5, 1000);
			ST_BYTE fuccode = pbuf[3];
			if (fuccode == 0x01 && pbuf[2] == 0x0c) //YX
				len = pbuf[2] + 1;
			else if(fuccode == 0x02 || fuccode == 0x03) //yc
				len = pbuf[2] + 1;
			else if(fuccode == 0x04 || fuccode == 0x05) //遥控的返回
				len = pbuf[2] + 1;
			else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
				//读数据返回
				len = pbuf[2] + 5;
			else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].writeCode)
				//写数据返回
				len = 8;
			else
			{
				this->ShowRecvFrame(pbuf, len);
				this->GetCurPort()->Clear();
			}
			len = this->GetCurPort()->PickBytes(pbuf, len, 1000);
			if(this->GetCurPort()->ReadBytes(pbuf, len) == len)
			{
				readed = len;
				return;
			}
			else
			{
				this->ShowRecvFrame(pbuf, len);
				this->GetCurPort()->Clear();
			}
		}
		else
		{
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
		}
	}
}

bool CJD200::OnSend()
{
	if (this->GetCurPort())
		this->GetCurPort()->Clear();
	ST_BYTE buf[2] = {0xEB, 0x90};
	this->Send(buf, sizeof(buf));
	Thread::SLEEP(200);

	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)  //遥控选择
			{
				if(m_curTask.taskAddr1 == 0xff) //按钮遥控
				{
					SendYK(0x04, m_curTask.taskAddr, (ST_BYTE)m_curTask.taskValue);
					m_nykpoint = m_curTask.taskAddr;
					m_nison    = m_curTask.taskValue;
					m_bbtncontrol = 1;
				}
				else if(m_curTask.taskAddr1 == 0xfe) //0变1
				{
					SendYK(0x04,m_curTask.taskAddr,(ST_BYTE)1);
				}
				else
				{
					SendYK(0x04,m_curTask.taskAddr,(ST_BYTE)m_curTask.taskValue);
				}
				m_nWaitTime = 2000;
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 1) //遥控执行
			{
				if (m_curTask.taskAddr1 == 0xfe) //0变1
				{
					SendYK(0x05,m_curTask.taskAddr,(ST_BYTE)1);
				}
				else
				{
					SendYK(0x05,m_curTask.taskAddr,(ST_BYTE)m_curTask.taskValue);
				}
				m_nWaitTime = 2000;
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 2) //遥控取消
			{
				m_curTask.taskResult.resultCode = 0;
				m_curTask.taskResult.resultDesc[0] = 'O';
				m_curTask.taskResult.resultDesc[1] = 'K';
				m_curTask.taskResult.resultDesc[2] = '\0';
				Transfer(&m_curTask);
				return false;
			}
			else if(m_curTask.taskCmdCode == 3) //复归
			{
				SendYK(0x04,0x00,0x00);
				m_nWaitTime = 4000;
				m_bTask = true;
				return true;
			}
		}
		else if(!strcmp(m_curTask.taskCmd,"singleread"))
		{
			if(m_curTask.taskCmdCode == 0)
			{
				ReadFix();
				m_nWaitTime = 4000;
				m_bTask = true;
			}
			else return false;
		}
		else if(!strcmp(m_curTask.taskCmd, "multiread"))
		{
			if(m_curTask.taskCmdCode == 0)
			{
				ReadFix();
				m_nWaitTime = 4000;
				m_bTask = true;
			}
			else return false;
		}
		else if(!strcmp(m_curTask.taskCmd, "singlewrite"))
		{
			if(m_curTask.taskCmdCode == 0)   //写单点定值
			{
				SendSingleWriteCmd(m_curTask.taskAddr,(ST_UINT16)m_curTask.taskValue);
				m_nWaitTime = 4000;
				m_bTask = true;
			}
			else return false;
		}
		else if(!strcmp(m_curTask.taskCmd, "multiwrite"))
		{
			return false;
		}
		return true;
	}
	else
	{
		time_t timeobject = time(0);
		if (timeobject > 0 && fabs (timeobject - m_tmSend) > 600)
		{
			TimeSync();
			m_tmSend = timeobject;
			return true;
		}

		const DeviceInfo * info = this->GetDevice()->GetDeviceInfo();
		if(info->DataAreasCount > 0)
		{
			if (m_readIndex >= info->DataAreasCount)
				m_readIndex = 0;
			m_curreadIndex = m_readIndex++;

			SendReadCmd (
				info->DataAreas[m_curreadIndex].readCode,
				info->DataAreas[m_curreadIndex].addr,
				info->DataAreas[m_curreadIndex].len
			);
			return true;
		}
		return true;
	}
}

void  CJD200::SendReadCmd(ST_BYTE code, ST_UINT16 readAddr, ST_UINT16 count)
{
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x05;
	sendbuf[3] = code;
	sendbuf[4] = iscleansoeflag;
	*(ST_UINT16*)(sendbuf + 5) = get_crc16 (sendbuf, 5);
	this->Send(sendbuf, 7);
	iscleansoeflag = 0x00;
}

void  CJD200::ReadFix(void)
{
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x05;
	sendbuf[3] = 0x03;
	sendbuf[4] = 0x00;
	*(ST_UINT16*)(sendbuf + 5) = get_crc16 (sendbuf, 5);
	this->Send(sendbuf, 7);
}

void  CJD200::SendSingleWriteCmd(ST_UINT16 readAddr, ST_UINT16 wValue)
{
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x09;
	sendbuf[3] = 0x08;
	sendbuf[4] = readAddr;
	FillWORD(sendbuf + 5, wValue);
	FillWORD(sendbuf + 7, 0x0000);
	*(ST_UINT16*)(sendbuf + 9) = get_crc16 (sendbuf, 9);
	this->Send(sendbuf, 11);
}

void CJD200::FillWORD (ST_BYTE* buf, ST_UINT16 v)
{
	ST_BYTE* pv = (ST_BYTE*)&v;
	buf[0] = pv[1];
	buf[1] = pv[0];
}

bool CJD200::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
	if(m_bTask) {
		if(!strcmp(m_curTask.taskCmd, "singlewrite") || !strcmp(m_curTask.taskCmd, "multiwrite"))
		{
			m_curTask.taskResult.resultCode    = 0;
			m_curTask.taskResult.resultDesc[0] = 'O';
			m_curTask.taskResult.resultDesc[1] = 'K';
			m_curTask.taskResult.resultDesc[2] = '\0';
			Transfer(&m_curTask);
			Memset  (&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
		{
			if(m_bbtncontrol == 1)
			{
				ST_BYTE buf[2] = {0xEB, 0x90};
				this->Send(buf, 2);
				Thread::SLEEP(200);
				SendYK(0x05, m_nykpoint, m_nison);
				m_curTask.taskResult.resultCode = 0;
				Transfer(&m_curTask);
				Memset  (&m_curTask, 0, sizeof(m_curTask));
				m_bbtncontrol = 2;
				Thread::SLEEP(500);
			}
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDesc[0] = 'O';
			m_curTask.taskResult.resultDesc[1] = 'K';
			m_curTask.taskResult.resultDesc[2] = '\0';
			Transfer(&m_curTask);
			Memset  (&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd, "singleread"))
		{
		//	m_curTask.taskResult.resultCode = 0;
		//	m_curTask.taskResult.resultValue = pbuf[3]+pbuf[4]*256;
		//	Transfer(&m_curTask);
		//	Memset  (&m_curTask, 0, sizeof(m_curTask));
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDesc[0] = 'O';
			m_curTask.taskResult.resultDesc[1] = 'K';
			m_curTask.taskResult.resultDesc[2] = '\0';
			Transfer(&m_curTask);
			Memset  (&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd, "multiread"))
		{
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDataLen = ((pbuf[2] - 5) / 3) * 2;
			for(int i = 0; i < (pbuf[2] - 5) / 3; i++)
			{
				m_curTask.taskResult.resultData[2 * i] = pbuf[4+3*i+1];
				m_curTask.taskResult.resultData[2*i+1] = pbuf[4+3*i+2];
			}
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDesc[0] = 'O';
			m_curTask.taskResult.resultDesc[1] = 'K';
			m_curTask.taskResult.resultDesc[2] = '\0';
			Transfer(&m_curTask);
			Memset  (&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
	}
	if (pbuf[2] == 0x1e) //说明产生SOE
	{
		Explainsoe (&pbuf[4], pbuf[2] - 4);
		iscleansoeflag = 0xff;
		return true;
	}
	ST_BYTE readcode = pbuf[3];
	ST_BYTE count    = pbuf[2] - 4;
	if (readcode == 1)
		ProcessMemory0x01(&pbuf[4], count);
	else if (readcode == 2)
		ProcessMemory0x02(&pbuf[4], count);
	return true;
}

void  CJD200::ProcessMemory0x01(ST_BYTE * buf, ST_BYTE count)
{
	int nIndexYX = m_curreadIndex * 100;
	for(int i = 0; i < count - 2; i++)
	{
		if (i < 1) {
			for(int k = 0; k < 8; k++)
			{
				ST_BYTE byValue = ((0x01 << k) & buf[i]) ? 0x00 : 0x01;
				this->UpdateValue(nIndexYX++, (ST_BYTE)byValue);
			}
		}
		else {
			for(int k = 0; k < 8; k++)
			{
				ST_BYTE byValue = ((0x01 << k) & buf[i]) ? 0x01 : 0x00;
				this->UpdateValue(nIndexYX++, (ST_BYTE)byValue);
			}
		}
	}
}

void  CJD200::ProcessMemory0x02(ST_BYTE * buf, ST_BYTE count)
{
	int nIndexYX = m_curreadIndex * 100;
	for(int i = 0; i < (count - 2) / 2; i++)
	{
		float fValue = buf[2*i] + buf[2*i+1] * 256;
		if(this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[i].coeficient != 0)
		{
			if (i == 8 || i == 11) //有符号 //有功的符号
			{
				if (buf[24] + buf[25] * 256)
					fValue *= -1;
			}
			else if (i == 9)
			{
				if (buf[26] + buf[27] * 256)
					fValue *= -1;
			}

			fValue *= this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[i].coeficient;

		}
		this->UpdateValue(nIndexYX++, (float)fValue);
	}
}

void  CJD200::ProcessMemory0x03(ST_BYTE * buf, ST_BYTE count)
{
	int nIndexYX = m_curreadIndex * 100;
	for(int i = 0; i < (count - 2) / 2; i++)
	{
		float fValue = buf[2 * i] + buf[2 * i + 1] * 256;
		if(this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[i].coeficient != 0)
			fValue *= this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[i].coeficient;
		this->UpdateValue(nIndexYX++, (float)fValue);
	}

}

void CJD200::SendYK(ST_BYTE bycid, ST_UINT16 writeAddr, bool bIsOn)
{
	ST_BYTE sendbuf[32] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x09;
	sendbuf[3] = bycid;
	sendbuf[4] = writeAddr;
	if(writeAddr == 0x00)
	{
		sendbuf[5] = 0x00;
		sendbuf[6] = 0x00;
		sendbuf[7] = 0x00;
		sendbuf[8] = 0x00;
	}
	else
	{
		if (bIsOn) {
			sendbuf[5] = 0x55;
			sendbuf[6] = 0x55;
			sendbuf[7] = 0x55;
			sendbuf[8] = 0x55;
		}
		else {
			sendbuf[5] = 0x33;
			sendbuf[6] = 0x33;
			sendbuf[7] = 0x33;
			sendbuf[8] = 0x33;
		}
	}
	*(ST_UINT16*)(sendbuf + 9) = get_crc16 (sendbuf, 9);
	this->Send(sendbuf, 11);
}

void CJD200::TimeSync()  //对时
{
	time_t t_now = time(0);
	if (t_now < 0) return;
	struct tm t_tm;
	DateTime::localtime (t_now, t_tm);

	ST_BYTE data[16] = {0};
	data[ 0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[ 1] = 0x00;
	data[ 2] = 0x0C;  //数据长度
	data[ 3] = 0x00;  //功能码
	data[ 4] = t_tm.tm_year - 100;
	data[ 5] = t_tm.tm_mon  + 1;
	data[ 6] = t_tm.tm_mday;
	data[ 7] = t_tm.tm_hour;
	data[ 8] = t_tm.tm_min ;
	data[ 9] = t_tm.tm_sec ;
	data[10] = 0x00;
	data[11] = 0x00;
	*(ST_UINT16*)(data + 12) = get_crc16 (data, 12);
	this->Send(data, data[2] + 2);
}

#pragma pack(push,1)	// 紧凑对齐

struct SOE {
	inline ST_UINT16 msec() const { return msec_b_h * 256 + msec_b_l; }

	ST_UINT8  event_type;
	ST_UINT8  mon;
	ST_UINT8  day;
	ST_UINT8  hour;
	ST_UINT8  min;
	ST_UINT8  sec;
	ST_UINT8  msec_b_h;
	ST_UINT8  msec_b_l;
	ST_UINT8  value;
	ST_UINT16 values[8];
};

#pragma pack(pop)		// 恢复默认内存对齐

void CJD200::Explainsoe(ST_BYTE * buf, int count)
{
	const SOE * soeptr = (const SOE *) buf;
	// int iyear,imonth,iday,ihour,iminute,isecond,imillsecond;
	// // CTime nownew=CTime::GetCurrentTime();
	// // iyear       = nownew.GetYear();
	// imonth      = buf[2];
	// iday        = buf[3];
	// ihour       = buf[4];
	// iminute     = buf[5];
	// isecond     = buf[6];
	// imillsecond = buf[7] * 256 + buf[8];
	// CString soetypevalue = soestr[buf[0]-1];
	// CString  eventstate,eventtime;
	// eventtime.Format("%d年%d月%d日%d时%d分%d秒%d毫秒",iyear,imonth,iday,ihour,iminute,isecond,imillsecond);
	// CString str,devname;
	// devname = this->GetDevice()->GetDeviceInfo()->DeviceName;
	float fvalue[8] = {0};
	if (soeptr->values[0] == 0xFFFF)
	{
		// str.Format("%s %s %s,无动作值 ",devname,eventtime,soetypevalue);
	}
	else
	{
		for (int i = 0; i < 4; i++) //开始的四个值都是0.01
		{
			fvalue[i] = soeptr->values[i] * 0.01;//(buf[9+2*i] + buf[10+2*i] * 256) * 0.01;
		}

		ST_BYTE SOELimitValue = this->GetDevice()->GetDeviceInfo()->DataAreas[0].writeCode;
		switch (SOELimitValue) {
			case 0:
			case 1: {
				for (int i = 4; i < 8; i++) //后面对
					fvalue[i] = soeptr->values[i] * 0.001;//(buf[9+2*i] + buf[10+2*i] * 256) * 0.001;
				// str.Format("%s %s %s 动作值为Uab1=%.2f,Ubc1=%.2f,Uca1=%.2f,Ua2=%.2f,Ia1=%.3f,Ib1=%.3f,Ic1=%.3f,Io1=%.3f",devname,eventtime,
				// soetypevalue,fvalue[0],fvalue[1],fvalue[2],fvalue[3],fvalue[4],fvalue[5],fvalue[6],fvalue[7]);
			} break;
			case 2:
			case 3:
			case 4: {
				for (int i = 4; i < 8; i++) //后面对
					fvalue[i] = soeptr->values[i] * 0.01;//(buf[9+2*i] + buf[10+2*i] * 256) * 0.01;
				// str.Format("%s %s %s 动作值为Uab1=%.2f,Ubc1=%.2f,Uca1=%.2f,Uo2=%.2f,Uab2=%.2f,Ubc2=%.2f,Uca2=%.2f,Uo2=%.2f",devname,eventtime,
				// soetypevalue,fvalue[0],fvalue[1],fvalue[2],fvalue[3],fvalue[4],fvalue[5],fvalue[6],fvalue[7]);
			} break;
		}
	//	str.Format("%s %s %s 动作值为%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",devname,eventtime,
		//	soetypevalue,fvalue[0],fvalue[1],fvalue[2],fvalue[3],fvalue[4],fvalue[5],fvalue[6],fvalue[7]);
	}
	// this->ReportEvent(iyear,imonth,iday,ihour,iminute,isecond,imillsecond,str,"soesj");
	//
	struct tm t_tm;
	DateTime::localtime (time(0), t_tm);

	ProtocolTask task;
	Strcpy(task.taskCmd,"SOE");
	task.isTransfer     = true;
	task.transChannelId = -1;
	task.channelId      = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
	task.deviceId       = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.taskCmdCode    = 0;
	task.taskParamLen   = 14;
	task.taskAddr       = soeptr->event_type + 300;
	task.taskValue      = (soeptr->value == 0x55 ? 1: 0);
	task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.ignoreBack     = 1;
	task.taskTime       = 1000;
	task.taskParam[0]   = (t_tm.tm_year + 1900) % 256;
	task.taskParam[1]   = (t_tm.tm_year + 1900) / 256;
	task.taskParam[2]   = soeptr->mon;
	task.taskParam[3]   = soeptr->day;
	task.taskParam[4]   = soeptr->hour;
	task.taskParam[5]   = soeptr->min;
	task.taskParam[6]   = soeptr->sec;
	task.taskParam[7]   = soeptr->msec_b_l;
	task.taskParam[8]   = soeptr->msec_b_h;
	task.taskParam[9]   = task.taskValue;
	task.taskParam[10]  = task.taskAddr  % 256;
	task.taskParam[11]  = task.taskAddr  / 256;
	task.taskParam[12]  = task.taskAddr1 % 256;
	task.taskParam[13]  = task.taskAddr1 / 256;
	Transfer(&task);
}
