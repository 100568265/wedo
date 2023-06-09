

#include "CPU_flowmeter.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include <math.h>

CPU_flowmeter::CPU_flowmeter()
{
    //ctor

}

CPU_flowmeter::~CPU_flowmeter()
{
    //dtor
}
CPU_flowmeter* CreateInstace()
{
    return new CPU_flowmeter();
}
void	CPU_flowmeter::Init()
{

}
void	CPU_flowmeter::Uninit()
{

}
void	CPU_flowmeter::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	if (this->GetCurPort())
	{
		ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
        interval = (interval > 2000 ? interval : 2000);

        ST_INT len = this->GetCurPort()->PickBytes(pbuf, 8, interval);
        if(len < 8) {
            ShowMessage ("Insufficient data length length");
            this->GetCurPort()->Clear();
            return;
        }
        ST_INT star = 0;
        for(; star < len; ++star) {
            if(pbuf[star] == 0xDD)
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
        len = this->GetCurPort()->PickBytes(pbuf, 8, 2000);
        if(pbuf[0]==0xDD&&pbuf[1]==(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
        {
            if(pbuf[2] == 0x31)
            {
                int t_len = (pbuf[3]*256+pbuf[4])+7;
                this->GetCurPort()->ReadBytes(pbuf, t_len);
                readed = t_len;
                return;
            }
            else
            {
                ShowMessage("Function code error");
            }
        }
        else
        {
            ShowMessage("Device address does not match");
            this->GetCurPort()->Clear();
            return;
        }

	}
}
ST_BOOLEAN	CPU_flowmeter::OnSend()
{

	return true;
}
ST_BOOLEAN	CPU_flowmeter::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
	ST_DOUBLE DataValue;

	DataValue = ChangeFloat(&pbuf[7],6);//总量
	this->UpdateValue(0,DataValue);

	DataValue = ChangeInt(&pbuf[11],6);//余量
	this->UpdateValue(1,DataValue);

	DataValue = ChangeFloat(&pbuf[17],4);//标况流量
	this->UpdateValue(2,DataValue);

	DataValue = ChangeFloat(&pbuf[21],4);//工况流量
	this->UpdateValue(3,DataValue);

	DataValue = ChangeFloat(&pbuf[25],4);//温度
	this->UpdateValue(4,DataValue);

	DataValue = ChangeFloat(&pbuf[29],4);//压力值
	this->UpdateValue(5,DataValue);

	ST_BYTE tem = 0x01;
	for(ST_INT i = 0; i < 5; i++)//状态标志
	{

		this->UpdateValue(i+6,(pbuf[33]&tem)>>i);
		tem <<= 1;
	}

	return true;
}
ST_BOOLEAN	CPU_flowmeter::IsSupportEngine(ST_INT engineType)
{
	return 1;
}
ST_DOUBLE CPU_flowmeter::ChangeFloat(ST_BYTE* pbuf, ST_INT len)
{
	if (!(pbuf[0]>>7))//阶符为0，阶符为正
	{
		ST_INT power = pbuf[0]&(~0x80);//阶
		ST_ULONG data = 0;
		data = pbuf[1]&(~0x80);
		data <<= 8;
		data += pbuf[2];
		data <<= 8;
		data += pbuf[3];

		datavalue = (data / 8388608.00) * pow(2.0,power);





	}
	else//阶符为1，负数
	{
		ST_INT power =	(~pbuf[0] + 1)&(~0x80);
		ST_ULONG data = 0;
		data = (~pbuf[1] +1)&(~0x80);
		data <<= 8;
		data += (~pbuf[2] + 1);
		data <<= 8;
		data += (~pbuf[3] + 1);

		datavalue = (data / 8388608.00) * pow(2.0,-power);



	}
	if (len == 4)
	{
		if (!(pbuf[1]>>7))
		{

			return datavalue;
		}
		else
		{

			return -datavalue;
		}
	}
	if (len == 6)
	{
		ST_DOUBLE tem_value = (*(pbuf-2)>>4)*1000 + ((*(pbuf-2))&0x0F)*100 + (*(pbuf-1)>>4)*10 + ((*(pbuf-1))&0x0F);
		if (!(pbuf[1]>>7))
		{
			datavalue = datavalue+tem_value*pow(10,6);
			return datavalue;
		}
		else
		{
			datavalue = datavalue+tem_value*pow(10,6);
			return -datavalue;
		}


	}
	return 0;
}

ST_DOUBLE CPU_flowmeter::ChangeInt(ST_BYTE *pbuf,ST_INT len)
{
    ST_INT i = 0;
	ST_ULONG data = 0;
	if (pbuf[0] != 0x00)
	{

		for (; i < len-2; i++)
		{

			data += pbuf[i+1];
			data <<= 8;
		}
		data += pbuf[i+1];
		 datavalue = data;
		 datavalue = -datavalue;
	}
	else
	{
		for (; i < len-2; i++)
		{
			data += pbuf[i+1];
			data <<= 8;
		}
		data += pbuf[i+1];
		datavalue = data;
	}
	return datavalue;
}









