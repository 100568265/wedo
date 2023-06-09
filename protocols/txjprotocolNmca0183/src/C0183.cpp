#include "C0183.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"



const double pi = 3.14159265358979324;
const double a = 6378245.0;
const double ee = 0.00669342162296594323;
const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;
C0183::C0183()
{
    //ctor
}

C0183::~C0183()
{
    //dtor
}
C0183* CreateInstace()
{
    return new C0183();
}

void	C0183::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
	if(this->GetCurPort())
	{
		int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		if (lineInterval < 3000) lineInterval = 3000;
		int	len = this->GetCurPort()->PickBytes(pbuf, 10, lineInterval);
		if (len >= 10)
		{
			int star = 0;
			for(; star < len; ++star)
			{
				if(pbuf[star] == 0x24)
					break;
			}
			if(star == len)
			{
				ShowMessage("Is Messy Code!");
//				this->ShowReceiveFrame(pbuf,len);
				this->GetCurPort()->Clear();
				return;
			}
			if (star > 0)
			{
				this->GetCurPort()->ReadBytes(pbuf, star);
			}
			len = this->GetCurPort()->PickBytes(pbuf, 20, lineInterval);
			if (len < 20) {
				ShowMessage("Insufficient data length!");
//				this->ShowReceiveFrame(pbuf, len);
				this->GetCurPort()->Clear();
				return;
			}
			//24 47 4E 52 4D 43 $GNRMC
			if (pbuf[1]==0x47 && pbuf[2]==0x4E && pbuf[3]==0x52 && pbuf[4]==0x4D && pbuf[5]==0x43)
			{
				if (!(pbuf[17]=='A'))
				{
					ShowMessage("Unable to locate correctly");
					this->GetCurPort()->Clear();
					return;
				}
				len = this->GetCurPort()->ReadBytes(pbuf,45,lineInterval);
				if (len == 45)
				{
					readed = len;
					this->GetCurPort()->Clear();
					return;
				}
				else
				{
					ShowMessage("Insufficient data length!");
//					this->ShowReceiveFrame(pbuf,len);
					this->GetCurPort()->Clear();
				}
			}
			else
			{
				ShowMessage("head error");
				this->GetCurPort()->Clear();
				return;
			}

		}
		else
		{
			ShowMessage("Insufficient data length!");
//			this->ShowReceiveFrame(pbuf,len);
			this->GetCurPort()->Clear();
		}
	}
}
ST_BOOLEAN	C0183::OnSend()
{
    return true;
}
ST_BOOLEAN	C0183::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    if (pbuf[17]=='A')
	{
	/*
		long double latitude =(((pbuf[19]-'0')*1000000)+((pbuf[20]-'0')*100000)+((pbuf[21]-'0')*10000)+((pbuf[22]-'0')*1000)
						+((pbuf[24]-'0')*100)+((pbuf[25]-'0')*10)+((pbuf[26]-'0'))+((pbuf[27]-'0')*0.1)+((pbuf[28]-'0')*0.01))*0.001;

		long double longitude=(((pbuf[32]-'0')*1000000)+((pbuf[33]-'0')*100000)+((pbuf[34]-'0')*10000)+((pbuf[35]-'0')*1000)
						+((pbuf[36]-'0')*100)+((pbuf[38]-'0')*10)+((pbuf[39]-'0'))+((pbuf[40]-'0')*0.1)+((pbuf[41]-'0')*0.01)+((pbuf[42]-'0')*0.001))*0.01;
		*/
		long double latitude =(((pbuf[19]-'0')*100000000)+((pbuf[20]-'0')*10000000)+((pbuf[21]-'0')*1000000)+((pbuf[22]-'0')*100000)
						+((pbuf[24]-'0')*10000)+((pbuf[25]-'0')*1000)+((pbuf[26]-'0')*100)+((pbuf[27]-'0')*10)+((pbuf[28]-'0')*0.1))*0.00001;

		long double longitude=(((pbuf[32]-'0')*1000000000)+((pbuf[33]-'0')*100000000)+((pbuf[34]-'0')*10000000)+((pbuf[35]-'0')*1000000)
						+((pbuf[36]-'0')*100000)+((pbuf[38]-'0')*10000)+((pbuf[39]-'0')*1000)+((pbuf[40]-'0')*100)+((pbuf[41]-'0')*10)+((pbuf[42]-'0')))*0.00001;

		char latbuf[80];
		sprintf(latbuf,"lat_long:%.5f,%.5f",latitude,longitude);
		ShowMessage(latbuf);

        double lat_gather = 0.0, long_gather = 0.0, marsLat = 0, marsLon = 0;
		int buffer[6] = {0};
		TranslateCoordilate(latitude,longitude,buffer);
//        TranslateCoordilate(2222.40683,11333.55119,buffer);
		ToDec(buffer,lat_gather,long_gather);
		transform2Mars(lat_gather,long_gather,marsLat,marsLon);
		double tmp_lat = marsLat;
		double tmp_lon = marsLon;
		this->UpdateValue(0,(double)marsLon);
		this->UpdateValue(1,(double)marsLat);
		char printfMSG[80];
		sprintf(printfMSG,"%.6f,%.6f",marsLon,marsLat);
		ShowMessage(printfMSG);
	}
	return true;
}
ST_BOOLEAN	C0183::IsSupportEngine(ST_INT engineType)
{
    return true;
}

bool C0183::outOfChina(double lat, double lon)
{
	if (lon < 72.004 || lon > 137.8347)
		return true;
	if (lat < 0.8293 || lat > 55.8271)
		return true;
	return false;
}

double C0183::transformLat(double x, double y)
{
	double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(abs(x));
	ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
	ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
	ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
	return ret;
}

double C0183::transformLon(double x, double y)
{
	double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(abs(x));
	ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
	ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
	ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
	return ret;
}

/**
* 地球坐标转换为火星坐标
* World Geodetic System ==> Mars Geodetic System
*
* @param wgLat  地球坐标
* @param wgLon
*
* mglat,mglon 火星坐标
*/
void C0183::transform2Mars(double wgLat, double wgLon,double &mgLat,double &mgLon)
{
    double dLat = transformLat(wgLon - 105.0, wgLat - 35.0);
    double dLon = transformLon(wgLon - 105.0, wgLat - 35.0);
    double radLat = wgLat / 180.0 * pi;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
    mgLat = wgLat + dLat;
    mgLon = wgLon + dLon;
}

void C0183::TranslateCoordilate(long double latitude,long double longitude,int *buf)
{
	//经度
	//度
	buf[0] = (int)(latitude / 100);
	//分
	buf[1] = (int)(latitude - (buf[0] * 100));
	//秒
	buf[2] = (int)((latitude - (buf[0] * 100) - buf[1]) * 60);

	//维度
	//度
	buf[3] = (int)(longitude / 100);
	//分
	buf[4] = (int)(longitude - (buf[3] * 100));
	//秒
	buf[5] = (int)((longitude - (buf[3] * 100) - buf[4]) * 60);
}





void C0183::ToDec(int *buf,double &x, double &y)
{
	long double m_longitude = 0.0,m_latitude = 0.0;
	long double m_buf[6] = {0};
	for (int i=0;i<6;i++)
	{
		m_buf[i] = buf[i];
	}
	m_latitude	 = m_buf[0] + ((m_buf[1]*1000000000000/60 + m_buf[2]*1000000000000/3600)/ 1000000000000.0);
	m_longitude  = m_buf[3] + ((m_buf[4]*1000000000000/60 + m_buf[5]*1000000000000/3600)/ 1000000000000.0);

	x = (double)m_latitude;
	y = (double)m_longitude;
}

