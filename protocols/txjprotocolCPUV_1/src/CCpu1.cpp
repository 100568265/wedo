#include "CCpu1.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include "math.h"
#include "cstring"

CCpu1::CCpu1()
{
    //ctor

}

CCpu1::~CCpu1()
{
    //dtor
}

CCpu1* CreateInstace()
{
    return new CCpu1();
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
/*
unsigned char get_check_sum (const void * buf, size_t len)
{
	unsigned char sum = 0;
	unsigned char * i = (unsigned char *)buf;
	while (len-- >0)
		sum += *i++;

	return sum;
}
*/
ST_BOOLEAN	CCpu1::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	CCpu1::Init()
{

}

void	CCpu1::Uninit()
{

}
void	CCpu1::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	if(! this->GetCurPort())
		return;
	ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 8, interval);
	if(len < 8) {
		ShowMessage ("Insufficient data length");
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
		ShowMessage ("Garbled code, clear buffer");
		this->GetCurPort()->Clear();
		return;
	}
	/////////////////////////////////////////////////////////////////////
    len = this->GetCurPort()->PickBytes(pbuf, 8, 2000);
	ST_BYTE fuccode = pbuf[2];
	if(fuccode==0x31)
	{
        ST_BYTE readCount = pbuf[4];
        len = readCount +7;
	}
	else
	{
        ShowMessage ("fuccode Error.");
        this->GetCurPort()->Clear();
	}
	int nlen = this->GetCurPort()->PickBytes(pbuf,len,interval);

	if(nlen == len)
    {
//        this->GetCurPort()->ReadBytes(pbuf,len);

        if (get_check_sum(pbuf, len - 2) == pbuf[len - 2])
        {
            if (this->GetCurPort()->ReadBytes(pbuf, len) == len)
            {
                readed = len;
                return;
            }
        }
        else
        {
            ShowMessage ("check Error.");
            this->GetCurPort()->Clear();
            return;
        }
    }
    else
    {
        ShowMessage ("Insufficient data length!");
        this->GetCurPort()->Clear();
    }

}

ST_BOOLEAN	CCpu1::OnSend()
{
	return true;
}


ST_BOOLEAN	CCpu1::OnProcess(ST_BYTE* pbuf,ST_INT len)
{

    ST_BYTE funAddr[5] = {0x02,0x03,0,0,0};
    ST_DOUBLE resData = 0;
	ST_INT  i_resData =0 ;
	ST_DOUBLE testData=0;
	ST_INT i = 0;
	ST_DOUBLE fixedFig = 8388608;

	for (i=0;i<5;i++)
	{
        if (funAddr[i]==pbuf[1])
            break;
	}
    if(i==5)
    return true;


			//总量
			i_resData = counterBcd(pbuf[5])*100+counterBcd(pbuf[6]);
			testData= countSixteen(pbuf[8],2) + countSixteen(pbuf[9],1)+ pbuf[10];
			resData = (testData /fixedFig)*pow(2,pbuf[7])+i_resData*1000000;
			this->UpdateValue(0+i*6,resData);

			//余量
			i_resData = countSixteen(pbuf[13],3)+countSixteen(pbuf[14],2)+countSixteen(pbuf[15],1)+pbuf[16];
			if (pbuf[11]==1)
			{
				i_resData = -i_resData;
			}
			this->UpdateValue(1+i*6,i_resData);

			//标况流量
			testData=countSixteen(pbuf[18],2) + countSixteen(pbuf[19],1)+ pbuf[20];
			resData = (testData /fixedFig)*pow(2,pbuf[17]);
			this->UpdateValue(2+i*6,resData);

			//工况流量
			testData=countSixteen(pbuf[22],2) + countSixteen(pbuf[23],1)+ pbuf[24];
			resData = (testData /fixedFig)*pow(2,pbuf[21]);
			this->UpdateValue(3+i*6,resData);

			//温度值
			testData=countSixteen(pbuf[26],2) + countSixteen(pbuf[27],1)+ pbuf[28];
			resData = (testData /fixedFig)*pow(2,pbuf[25]);
			this->UpdateValue(4+i*6,resData);

			//压力值
			testData=countSixteen(pbuf[30],2) + countSixteen(pbuf[31],1)+ pbuf[32];
			resData = (testData /fixedFig)*pow(2,pbuf[29]);
			this->UpdateValue(5+i*6,resData);

   //         break;



	return true;
}

ST_INT CCpu1::countSixteen(ST_BYTE aData,ST_INT len)
{
	ST_INT  resFig=aData;
	ST_INT i=0;
	for (i=0;i<len;++i)
	{
		resFig = resFig*16*16;
	}
	return resFig;
}

ST_INT CCpu1::counterBcd(ST_BYTE data_BCD)
{
    ST_INT x = 0;
    x = data_BCD>>4;

    return x=x*10+(data_BCD&=0x0f);
}

