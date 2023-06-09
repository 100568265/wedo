#include "GPRS.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include <time.h>

GPRS::GPRS()
{
    //ctor
}

GPRS::~GPRS()
{
    //dtor
}

void	GPRS::Init()
{

}
void	GPRS::Uninit()
{

}

// 获取异或校验值
inline ST_BYTE  checkXOR(ST_BYTE * puf,int len)
{
	ST_BYTE xorRes = puf[1];
	int checklen = len;
	for (int i = 0;i<checklen;i++)
	{
		xorRes ^= puf[i];
	}
	return xorRes;
}

GPRS* CreateInstace()
{
    return new GPRS();
}


void	GPRS::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{

    readed = 0;
	if(! this->GetCurPort())
		return;
    ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	if(len < 5) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	for(; star < len; ++star) {
		if(pbuf[star] == 0x53)
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
	len = 18;
	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 2000);
	if(nlen == len)
	{

//	    ST_BYTE cpuf = checkXOR(pbuf+2,14);
//	    if(cpuf == pbuf[16])
//	    {
            this->GetCurPort()->ReadBytes(pbuf, len);
            readed = len;
            return;
//	    }
//		else
//		{
//            ShowMessage("check error!");
//            this->GetCurPort()->Clear();
//            return;
//		}
	}
	else
	{
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
	}

}
ST_BOOLEAN	GPRS::OnSend()
{
    if (!curEngineType)
        sleep (1);     // 全双工通道时睡眠以空出CPU资源
    return true;
}
ST_BOOLEAN	GPRS::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    if(pbuf[0] == 0x53 &&pbuf[17]==0x41)
    {
        int m_hour = (pbuf[2]-0x30)*10+(pbuf[3]-0x30);
        int m_minute = (pbuf[4]-0x30)*10+(pbuf[5]-0x30);
        int m_second = (pbuf[6]-0x30)*10+(pbuf[7]-0x30);
        int m_day = (pbuf[8]-0x30)*10+(pbuf[9]-0x30);
        int m_month = (pbuf[10]-0x30)*10+(pbuf[11]-0x30);
        int m_year = (pbuf[12]-0x30)*1000+(pbuf[13]-0x30)*100+(pbuf[14]-0x30)*10+(pbuf[15]-0x30);

        struct tm  t_tm;
        memset (&t_tm, 0, sizeof(t_tm));
        t_tm.tm_hour = (pbuf[2]-0x30)*10+(pbuf[3]-0x30);
        t_tm.tm_min  = (pbuf[4]-0x30)*10+(pbuf[5]-0x30);
        t_tm.tm_sec  = (pbuf[6]-0x30)*10+(pbuf[7]-0x30);
        t_tm.tm_mday = (pbuf[8]-0x30)*10+(pbuf[9]-0x30);
        t_tm.tm_mon  = (pbuf[10]-0x30)*10+(pbuf[11]-0x30);
        t_tm.tm_year = (pbuf[12]-0x30)*1000+(pbuf[13]-0x30)*100+(pbuf[14]-0x30)*10+(pbuf[15]-0x30);

        // 错误丢弃, 因无校验码添加的错误判断
        if (t_tm.tm_sec > 59 || t_tm.tm_min > 59 || t_tm.tm_hour > 23)
            return true;
        if (t_tm.tm_mday > 31 || t_tm.tm_mday < 1 || t_tm.tm_mon > 11 || t_tm.tm_mon < 0 || t_tm.tm_year > 200)
            return true;

        time_t newtime_t = mktime(&t_tm);
        if (newtime_t <= 0)
            return true;

        stime(&newtime_t);
/*
        struct tm *p = new struct tm();
        struct timeval tv;
        struct timezone tz;
        gettimeofday (&tv , &tz);//获取时区保存tz中
        p->tm_year = m_year - 1900;
        p->tm_mon = m_month - 1;
        p->tm_mday = m_day;
        p->tm_hour = m_hour;
        p->tm_min = m_minute;
        p->tm_sec = m_second;
        time_t utc_t = mktime(p);
        delete(p);
        tv.tv_sec = utc_t;
        tv.tv_usec = 0;
        if (settimeofday(&tv, &tz) < 0)
        {
            ShowMessage("set time error!");
            return false;
        }
        return true;*/

    }
}
ST_BOOLEAN	GPRS::IsSupportEngine(ST_INT engineType)
{
    curEngineType = engineType;
    return true;
}
