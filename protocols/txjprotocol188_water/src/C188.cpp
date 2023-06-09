#include "C188.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
	ST_BYTE bySum = 0x00;
	for(int i = 0; i < len; ++i)
	{
		bySum += pbuf[i];
	}
	return  bySum;
}

inline unsigned char bcd2hex(unsigned char data)
{
	unsigned char temp;

	temp = ((data >> 4) * 10 + (data & 0x0f));
	return temp;
}

Protocol * CreateInstace()
{
	return new C188();
}

C188::C188()
{
    //ctor
}

C188::~C188()
{
    //dtor
}

void C188::Init()
{

}

void C188::Uninit()
{

}

bool C188::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void C188::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
		if(len < 12) {
			ShowMessage ("Insufficient data length");
			return;
		}
		ST_INT star = 0;
		for(; star < len; ++star) {
			if(pbuf[star] == 0x68)
				break;
		}
		if(len == star) {
			ShowMessage ("Garbled code, clear buffer.");
			this->GetCurPort()->Clear();
			return;
		}
		if(star > 0)
		{
			this->GetCurPort()->ReadBytes(pbuf, star);
		}
		len = pbuf[10] + 12;
		if((pbuf[0] == 0x68) && (pbuf[1] == 0x10))
		{

			if(this->GetCurPort()->ReadBytes(pbuf, 22) == 22)
			{
                if (memcmp(pbuf + 2, m_addrarea, sizeof(m_addrarea)))
				{
					this->ShowMessage("Address not match.");
					return;
				}
				if (get_check_sum(pbuf, 20) == pbuf[20])
				{
					readed = len;
					return;
				}
				else {
					ShowMessage("Check error!");
					this->ShowRecvFrame(pbuf,len);
					this->GetCurPort()->Clear();
					return;
				}
			}
		}
		else
		{
			ShowMessage("Sync header error.");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
			return;
		}
	}
}


bool C188::OnSend()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[12]-'0')*16 + (info.Addressex[13]-'0');
	m_addrarea[1] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
	m_addrarea[2] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
	m_addrarea[3] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[4] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[5] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[6] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

	ST_BYTE sendbuf[125];
	sendbuf[0] = 0x68;
	sendbuf[1] = 0x10;
	memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));
	sendbuf[9] = 0x01;
	sendbuf[10] = 0x03;
	sendbuf[11] = 0x1F;
	sendbuf[12] = 0x90;
	sendbuf[13] = 0x00;
//	sendbuf[14] = 0xD1;
    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 14; ++i)
	{
		bySum += sendbuf[i];
	}
	sendbuf[14] = bySum;
	sendbuf[15] = 0x16;

	this->Send(sendbuf, 16);
	return true;
}

bool C188::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
		if (pbuf[0] == 0x68 && pbuf[1]==0x10)
	{
		//当前累加值file:///home/work/wedo/comm/protocols/txjprotocol104_cong/txjprotocol104_cong.cbp

		//float vCurrent = (float)(bcd2hex(pbuf[15])+bcd2hex(pbuf[16])*100+bcd2hex(pbuf[17])*10000+((pbuf[14]&0x0f)*0.01)+((pbuf[14]>>4)*0.1));
		float vCurrent = (float)(((pbuf[14])&0x0f) + (((pbuf[14])&0xf0)>>4)*10 +
                               ((pbuf[15])&0x0f)*100 + (((pbuf[15])&0xf0)>>4)*1000 +
                               ((pbuf[16])&0x0f)*10000 + (((pbuf[16])&0xf0)>>4)*100000+
                               ((pbuf[17])&0x0f)*1000000 + (((pbuf[17])&0xf0)>>4)*10000000)*0.01;

		this->UpdateValue(0,float(vCurrent));
		return true;
	}
	return true;
}
