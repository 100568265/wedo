#include "CGPS.h"
//#include <cctype>
//#include <ctime>
#include <time.h>
#include <sys/time.h>
//#include "sysmalloc.h"
//#include "datetime.h"

CGPS::CGPS()
{
    //ctor
}

CGPS::~CGPS()
{
    //dtor
}

CGPS* CreateInstace()
{
    return new CGPS();
}

void CGPS::Init()
{

}

void CGPS::Uninit()
{
}

void CGPS::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	if(! this->GetCurPort())
        return;

    ST_INT len = this->GetCurPort()->PickBytes(pbuf, 24, 1000);
    if (len < 24) {
        this->ShowMessage ("Insufficient data length.");
        this->GetCurPort()->Clear();
        return;
    }

    ST_INT star = 0;
    for(; star < len; ++star) {
        if(pbuf[star] == 0x01)
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
    len = this->GetCurPort()->PickBytes(pbuf, 24, 100);
    if(pbuf[0]==0x01)
    {
        readed = this->GetCurPort()->ReadBytes(pbuf, 24);
        return;
    }
    else {
        this->ShowMessage ("Sync header error.");
        this->GetCurPort()->Clear();
    }
    /*
    if (pbuf[0] == 0x01) {
        len = this->GetCurPort()->PickBytes(pbuf, 24, 100);
        if (len < 24) {
            this->ShowMessage ("Insufficient data length.");
            this->GetCurPort()->Clear();
            return;
        }
        if (pbuf[23] == 0x0D && pbuf[24] == 0x0A) {
            readed = this->GetCurPort()->ReadBytes(pbuf, 24);
            return;
        }
    }
    else {
        this->ShowMessage ("Sync header error.");
        this->GetCurPort()->Clear();
    }*/
    return;
}

ST_BOOLEAN CGPS::OnSend()
{
    return true;
}

ST_BOOLEAN CGPS::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
    // 如2007年12月7日12时7分15秒
    // 01 323030373A 31323A 30373A 31323A 30373A 3135 2041/560D0A

//    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
//    01 32 30 31 39 3A 30 34 3A 31 38 3A 31 30 3A 33 36 3A 34 35 20 41 0D 0A
//        2  0  1  9  ： 0  4  ： 1   8  ：1   0  ：3   6  ： 4  5
//    2019年4月18日 10点38分2

    struct tm *p = new struct tm();
	struct timeval tv;
	struct timezone tz;
	gettimeofday (&tv , &tz);//获取时区保存tz中

    int c_year = (pbuf[1]-0x30)*1000+(pbuf[2]-0x30)*100+(pbuf[3]-0x30)*10+(pbuf[4]-0x30);
    int c_mon = (pbuf[6]-0x30)*10+(pbuf[7]-0x30);
    int c_day = (pbuf[9]-0x30)*10+(pbuf[10]-0x30);
    int c_hour = (pbuf[12]-0x30)*10+(pbuf[13]-0x30);
    int c_min = (pbuf[15]-0x30)*10+(pbuf[16]-0x30);
    int c_sec = (pbuf[18]-0x30)*10+(pbuf[19]-0x30);

	p->tm_year = c_year - 1900;
	p->tm_mon = c_mon-1;;
	p->tm_mday = c_day;
	p->tm_hour = c_hour;
	p->tm_min = c_min;
	p->tm_sec = c_sec;
	time_t utc_t = mktime(p);
//	delete(p);
	tv.tv_sec = utc_t;
	tv.tv_usec = 0;
	settimeofday (&tv, &tz);
/*
    if(settimeofday (&tv, &tz) < 0)
    {
        this->ShowMessage("set time error!");
    }
    else
    {
        this->ShowMessage("set time successful!");
    }*/
	return true;
}

ST_BOOLEAN	CGPS::IsSupportEngine(ST_INT engineType)
{
    return 1;
}
