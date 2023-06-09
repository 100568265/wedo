// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "DL645.h"
#include "Device.h"
#include "Channel.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

Protocol * CreateInstace()
{
	return new CDL645();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDL645::CDL645()
{
	m_readIndex = 0;
}

CDL645::~CDL645()
{

}

void CDL645::Init()
{}

void CDL645::Uninit()
{}

ST_BOOLEAN  CDL645::IsSupportEngine (ST_INT IsSupportEngine)
{
	return true;
}

inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
	ST_BYTE bySum = 0x00;
	for(int i = 0; i < len; ++i)
	{
		bySum += pbuf[i];
	}
	return  bySum;
}

void CDL645::OnRead(ST_BYTE* pbuf,int& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		if (lineInterval < 3000)
			lineInterval = 3000;
		int	len = this->GetCurPort()->PickBytes(pbuf, 12, lineInterval);
		if(len >= 12)
		{
			int star = 0;
			for(; star < len; ++star)
			{
				if(pbuf[star] == 0x68)
					break;
			}
			if(len == star) {
				this->GetCurPort()->Clear();
				return;
			}
			if(star > 0) {
			//	this->ShowRecvFrame(pbuf,len);
				this->GetCurPort()->ReadBytes(pbuf, star);
			}
			//ReadIndex = star;
			len = this->GetCurPort()->PickBytes(pbuf, 12, lineInterval);
			if((pbuf[0] == 0x68) && (pbuf[7] == 0x68) && pbuf[1]==sendbuf[1] && pbuf[2]==sendbuf[2] && pbuf[3]==sendbuf[3]
				&& pbuf[4]==sendbuf[4] && pbuf[5]==sendbuf[5] && pbuf[6]==sendbuf[6])
			{
				int ndatalen = pbuf[9] + 12;
				if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) < ndatalen)
					return;

				if (get_check_sum(pbuf,ndatalen - 2) == pbuf[ndatalen - 2])
				{
					readed = ndatalen;
					return;
				}
				else {
					ShowMessage("Check error!");
					this->ShowRecvFrame(pbuf,len);
					this->GetCurPort()->Clear();
					return;
				}
			}
			else {
				ShowMessage("Address error!");
				this->ShowRecvFrame(pbuf, len);
				this->GetCurPort()->Clear();
				return;
			}
		}
	}
}

bool CDL645::OnSend()
{
	switch(m_readIndex) {
		case  0: ReadData(0x9010);//有功电能量
			break;
		case  1: ReadData(0x9110);//无功电能量
			break;
		case  2: ReadData(0x9020);//有功电能量
			break;
		case  3: ReadData(0x9120);//无功电能量
			break;
		case  4: ReadData(0xb611);//电压
			break;
		case  5: ReadData(0xb612);//电压
			break;
		case  6: ReadData(0xb613);//电压
			break;
		case  7: ReadData(0xb621);//电流
			break;
		case  8: ReadData(0xb622);//电流
			break;
		case  9: ReadData(0xb623);//电流
			break;
		case 10: ReadData(0xb630);//有功功率
			break;
		case 11: ReadData(0xb631);//有功功率
			break;
		case 12: ReadData(0xb632);//有功功率
			break;
		case 13: ReadData(0xb633);//有功功率
			break;
		case 14: ReadData(0xb640);//无功功率
			break;
		case 15: ReadData(0xb641);//无功功率
			break;
		case 16: ReadData(0xb642);//无功功率
			break;
		case 17: ReadData(0xb643);//无功功率
			break;
		case 18: ReadData(0xb650);//功率因数
			break;
		case 19: ReadData(0xb651);//功率因数
			break;
		case 20: ReadData(0xb652);//功率因数
			break;
		case 21: ReadData(0xb653);//功率因数
			break;
		default:
			break;
	}
	m_readIndex = (++m_readIndex % 22);//22
	return true;
}

inline uint8_t  bcd2bin8  (uint8_t  value)
{
	return (value >> 4) * 10 + (value & 0x0F);
}

inline uint16_t bcd2bin16 (uint16_t value)
{
	return (uint16_t)bcd2bin8(value >> 8) * 100 + (uint16_t)bcd2bin8(value & 0x00FF);
}

inline uint32_t bcd2bin32 (uint32_t value)
{
	return (uint32_t)bcd2bin16(value >> 16) * 10000 + (uint32_t)bcd2bin16(value & 0x0000FFFF);
}

bool CDL645::OnProcess(ST_BYTE* pbuf, int len)
{
	if(pbuf[8] == 0x81)
	{
		if(((pbuf[10]-0x33) == 0x10) && ((pbuf[11]-0x33) == 0x90))//有功电能
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
						   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
						   ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(0,float(fvalue));
		}
		else if(((pbuf[10]-0x33) == 0x10) && ((pbuf[11]-0x33) == 0x91))//无功电能
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
						   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
						   ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(1,float(fvalue));
		}
		if(((pbuf[10]-0x33) == 0x20) && ((pbuf[11]-0x33) == 0x90))//有功电能
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
						   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
						   ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(24,float(fvalue));
		}
		else if(((pbuf[10]-0x33) == 0x20) && ((pbuf[11]-0x33) == 0x91))//无功电能
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
						   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
						   ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(25,float(fvalue));
		}
		if(((pbuf[10]-0x33) >= 0x11) && ((pbuf[10]-0x33) <= 0x13) && ((pbuf[11]-0x33) == 0xb6))//电压
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
					   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*1;
			this->UpdateValue(2+(pbuf[10]-0x33)-0x11,float(fvalue));
		}
		else if(((pbuf[10]-0x33) >= 0x21) && ((pbuf[10]-0x33) <= 0x23) && ((pbuf[11]-0x33) == 0xb6))//电流
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.01;
			this->UpdateValue(5+(pbuf[10]-0x33)-0x21,float(fvalue));
		}
		else if(((pbuf[10]-0x33) >= 0x30) && ((pbuf[10]-0x33) <= 0x33) && ((pbuf[11]-0x33) == 0xb6))//有功功率
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
					   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
					   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;

			this->UpdateValue( 8+(pbuf[10]-0x33)-0x30, (((pbuf[14]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
		}
		else if(((pbuf[10]-0x33) >= 0x40) && ((pbuf[10]-0x33) <= 0x43) && ((pbuf[11]-0x33) == 0xb6))//无功功率
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0x70)>>4)*1000)*0.01;

			this->UpdateValue(12+2+(pbuf[10]-0x33)-0x40, (((pbuf[13]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
		}
		else if(((pbuf[10]-0x33) >= 0x50) && ((pbuf[10]-0x33) <= 0x53) && ((pbuf[11]-0x33) == 0xb6))
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
					   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.001;
			this->UpdateValue(16+2+2+(pbuf[10]-0x33)-0x50,float(fvalue));
		}
	}
	return true;
}

void    CDL645::ReadData(ST_UINT16 wAddr)
{
//	ST_BYTE sendbuf[256];
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[ 0] = 0x68;
	sendbuf[ 1] = (info.Addressex[10]-'0') * 16 + (info.Addressex[11]-'0');
	sendbuf[ 2] = (info.Addressex[ 8]-'0') * 16 + (info.Addressex[ 9]-'0');
	sendbuf[ 3] = (info.Addressex[ 6]-'0') * 16 + (info.Addressex[ 7]-'0');
	sendbuf[ 4] = (info.Addressex[ 4]-'0') * 16 + (info.Addressex[ 5]-'0');
	sendbuf[ 5] = (info.Addressex[ 2]-'0') * 16 + (info.Addressex[ 3]-'0');
	sendbuf[ 6] = (info.Addressex[ 0]-'0') * 16 + (info.Addressex[ 1]-'0');
	sendbuf[ 7] = 0x68;
	sendbuf[ 8] = 0x01;
	sendbuf[ 9] = 0x02;
	sendbuf[10] =  (wAddr & 0x00ff)       + 0x33;
	sendbuf[11] = ((wAddr & 0xff00) >> 8) + 0x33;
    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 12; ++i)
		bySum += sendbuf[i];

	sendbuf[12] = bySum;
	sendbuf[13] = 0x16;
	this->Send(sendbuf, 14);
}
