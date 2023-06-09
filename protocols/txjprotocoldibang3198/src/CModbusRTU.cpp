#include "CModbusRTU.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include "datetime.h"
#define sDebug	if (true) wedoDebug (SysLogger::GetInstance()).noquote


CModbusRTU::CModbusRTU()
{
    //ctor
}

CModbusRTU::~CModbusRTU()
{
    //dtor
}

CModbusRTU* CreateInstace()
{
    return new CModbusRTU();
}

ST_BOOLEAN	CModbusRTU::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	CModbusRTU::Init()
{
    m_curreadIndex = 0;
    countDataNum =0;
    countAllData = 0;
    th_count = 0;
}

void	CModbusRTU::Uninit()
{

}


void	CModbusRTU::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
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
	interval = (interval > 3000 ? interval : 3000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 9, interval);
	if(len < 9) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	for(; star < len; ++star) {
		if(pbuf[star] == 0x3D)
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
	len = this->GetCurPort()->PickBytes(pbuf, 9, 3000);

	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 3000);
	if(pbuf[0] == 0x3D && pbuf[4]==0x2E)
	{

		this->GetCurPort()->ReadBytes(pbuf, len);
		readed = len;
		return;
	}
	else
	{
		ShowMessage ("data error.");
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN	CModbusRTU::OnSend()
{
	return true;
}

ST_BOOLEAN	CModbusRTU::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    	if (pbuf[0] == 0x3D && pbuf[4] == 0x2E)
        {
            float fvalue = (pbuf[8]-0x30)*1000+(pbuf[7]-0x30)*100+(pbuf[6]-0x30)*10+(pbuf[5]-0x30)+
			(pbuf[3]-0x30)*0.1+(pbuf[2]-0x30)*0.01+(pbuf[1]-0x30)*0.001;

         //   countAllData = fvalue + countAllData;
            this->UpdateValue(0, fvalue);
            if(countAllData == fvalue && countAllData !=0)
            {
                countDataNum++;
            }
            if(countDataNum ==20)
            {
                    th_count ++;
                    countDataNum =0;
                    this->UpdateValue(1,th_count);
                    return true;

            }
            countAllData =  fvalue;

        }

	return true;
}

