#include "CWater.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
CWater::CWater()
{
    //ctor
}

CWater::~CWater()
{
    //dtor
}
void	CWater::Init()
{

}
void	CWater::Uninit()
{

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

void	CWater::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
	if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 13, 2000);
		if(len < 13) {
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
		if((pbuf[0] == 0x68) && (pbuf[5] == 0x68))
		{

			if(this->GetCurPort()->ReadBytes(pbuf, 13) == 13)
			{
                if (memcmp(pbuf + 2, m_addrarea, sizeof(m_addrarea)))
				{
					this->ShowMessage("Address not match.");
					return;
				}
				if (get_check_sum(pbuf, 11) == pbuf[11])
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
ST_BOOLEAN	CWater::OnSend()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	m_addrarea[0] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[1] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[2] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[3] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

	ST_BYTE sendbuf[125];
	sendbuf[0] = 0x68;
	memcpy (sendbuf + 1, m_addrarea, sizeof(m_addrarea));
    sendbuf[5] = 0x02;
	sendbuf[6] = 0x00;
	sendbuf[7] = 0x00;
	sendbuf[8] = 0x00;
	sendbuf[9] = 0x00;
	ST_BYTE bySum =0;
	for(int i=0;i<10;i++)
	{
		bySum += sendbuf[i]; //总加和
	}
	sendbuf[10] = bySum;
	sendbuf[11] = 0x16;

	this->Send(sendbuf, 12);
	return true;
}
ST_BOOLEAN	CWater::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    	if (pbuf[0]==0x68 && pbuf[5]==0x68)
	{
		float fvalue= (float)((pbuf[10]*256*256*256)+
				(pbuf[9]*256*256)+
				(pbuf[8]*256)+
				pbuf[7])*0.1;
		this->UpdateValue(0,(float)fvalue);
	}
	return true;
}
ST_BOOLEAN	CWater::IsSupportEngine(ST_INT engineType)
{
    return 1;
}
