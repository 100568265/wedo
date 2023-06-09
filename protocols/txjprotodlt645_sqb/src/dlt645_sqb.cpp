// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "dlt645_sqb.h"
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

inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)//生成校验码 pubf:从机发送的报文
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
		if (lineInterval < 3000)//?
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
		//水表__________________________________________________________
		case  0: ReadData(0x1010);//当前总用水量（表码）
			break;
	}
	m_readIndex = (++m_readIndex % 1);
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
		if(((pbuf[10]-0x33) == 0x10) && ((pbuf[11]-0x33) == 0x10))//当前总用水量（表码）
		{
			float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
						   ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
						   ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
						   ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(0,float(fvalue));
		}	
	}
	return true;
}

void    CDL645::ReadData(ST_UINT16 wAddr)//主站请求
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

	sendbuf[12] = bySum;//bySum:校验码
	sendbuf[13] = 0x16;
	this->Send(sendbuf, 14);
}
