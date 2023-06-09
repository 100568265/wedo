#include "Cchangyang.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include <time.h>
#include <iostream>
#include <unistd.h>


Cchangyang::Cchangyang()
{
                //ctor
}

Cchangyang::~Cchangyang()
{
                //dtor
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


unsigned long GetTickCount()
{
  struct timeval tv;
  if( gettimeofday(&tv, NULL) != 0 )
    return 0;

  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void	Cchangyang::Init()
{

}

void	Cchangyang::Uninit()
{

}

Cchangyang* CreateInstace()
{
    return new Cchangyang();
}

ST_BOOLEAN	Cchangyang::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

//ST_BOOLEAN	Cchangyang::IsSupportEngine(ST_INT engineType)
//{
//    return engineType == EngineBase::ENGINE_FULLING;
//  //  return 1;
//}

void	Cchangyang::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;

    if(! this->GetCurPort ()) {
                return;
    }


    ST_INT      len = this->GetCurPort()->PickBytes(pbuf,9,1000);

        if(len < 9) {
                ShowMessage ("Insufficient data length");
//                this->ShowRecvFrame(pbuf,20);
//                ShowMessage ("end end end");
                this->GetCurPort()->Clear();
                return;
                }
        ST_INT star = 0;
        for(;star < len; star++)
        {
            if(pbuf[star] == 0xEB)
                break;
        }
        if(star == len)
        {
            ShowMessage("变长帧出现部分乱码，丢弃乱码");
//				this->ShowReceiveFrame(pbuf,len);
            this->GetCurPort()->Clear();
            return;
        }
        if(star>0)
        {
            ShowMessage("变长帧出现部分乱码，丢弃乱码");
//				this->ShowReceiveFrame(pbuf,len);
            this->GetCurPort()->ReadBytes(pbuf,star);
        }
        len = this->GetCurPort()->PickBytes(pbuf,9,1000);

        ST_INT ndatalen=((pbuf[1]+(pbuf[2]<<8))&0x01FF)+6;

        len = this->GetCurPort()->PickBytes(pbuf,ndatalen,1000);

        if(len == ndatalen)
        {
            if(this->GetCurPort()->ReadBytes(pbuf,ndatalen) == len)
            {
                readed = len ;
                return ;
            }
        }
        else
        {
            ShowMessage("数据长度不够帧头，继续接收，");
//				this->ShowReceiveFrame(pbuf,len);
            this->GetCurPort()->Clear();
        }

    //if (! this->IsOpened()) {
        //m_nStart = 0;
        //return;
   // }
}

ST_BOOLEAN	Cchangyang::OnSend()
{

    if (this->GetCurPort())
       this->GetCurPort()->Clear();

                //ST_BYTE sendbuf[256];

                if(SendState == 1)//上电启动肯定确认/否认
                {
                                Confirmframe();
                                SendState = 2;
                }

                if(SendState == 2)//总召唤
                {
                                CallAll();

                }
                if(SendState == 3)//心跳
                {
                                sleep(10);
                                HeartBeat();
                                sleep(10);
                                AdjustTime();


                }
    return true;
}

ST_BOOLEAN	Cchangyang::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
                ST_BYTE funcode = pbuf[4];
                switch (funcode)
                {
                case 0x11:
                                {
                                                SendState = 1;
                                }
                                  break;
                case 0x05:
                                {
                                                ST_BYTE value[256];
                                                ST_INT itemlen = (pbuf[5]<<8) + pbuf[6];

                                                for ( ST_INT i = 0;i<itemlen;i++)
                                                {
                                                                value[i]= pbuf[9+2*i]>>7;
                                                                this->UpdateValue(i,value[i]);
                                                }

                                }
                                  break;
                case 0x06:
                                {
                                                float value[256];
                                                ST_INT itemlen = (pbuf[5]<<8) + pbuf[6];
                                                for(ST_INT i = 0;i<itemlen;i++)
                                                {
                                                                value[i] = (float)((pbuf[9+6*i]<<24)+(pbuf[10+6*i]<<16)+(pbuf[11+6*i]<<8)+pbuf[12+6*i]);
                                                                this->UpdateValue(i,value[i]);
                                                }
                                }
                                  break;
                case 0x07:
                                {
                                                float value[256];
                                                ST_INT itemlen = (pbuf[5]<<8) + pbuf[6];
                                                for(ST_INT i = 0;i<itemlen;i++)
                                                {
                                                                value[i] = (float)((pbuf[9+6*i]<<24)+(pbuf[10+6*i]<<16)+(pbuf[11+6*i]<<8)+pbuf[12+6*i]);
                                                                this->UpdateValue(i,value[i]);
                                                }
                                }
                                  break;
                case 0x0B:
                                {
                                                SendState = 3;
                                }
                                  break;

                }
    return true;
}




void	Cchangyang::Confirmframe()//上电启动肯定确认/否认
{
                ST_BYTE sendbuf[16];
                sendbuf[0] = 0xEB;
                sendbuf[1] = 0x03;
                sendbuf[2] = 0;
                sendbuf[3] = 0xEB;
                sendbuf[4] = 0x12;
                sendbuf[5] = 0;
                sendbuf[6] = 0;
                ST_BYTE bySum = 0x00;
                for(int i = 0; i < 12; ++i)
                bySum += sendbuf[i];
                sendbuf[7] = bySum;
                sendbuf[8] = 0x16;
                this->Send(sendbuf,9);
}

void        Cchangyang::CallAll()//发送总召唤
{
                ST_BYTE sendbuf[16];
                sendbuf[0] = 0xEB;
                sendbuf[1] = 0x03;
                sendbuf[2] = 0;
                sendbuf[3] = 0xEB;
                sendbuf[4] = 0x0D;
                sendbuf[5] = 0;
                sendbuf[6] = 0;
                ST_BYTE bySum = 0x00;
                for(int i = 0; i < 12; ++i)
                bySum += sendbuf[i];
                sendbuf[7] = bySum;
                sendbuf[8] = 0x16;
                this->Send(sendbuf,9);
}

void         Cchangyang::HeartBeat()//发送心跳报文
{
                ST_BYTE sendbuf[16];
                sendbuf[0] = 0xEB;
                sendbuf[1] = 0x03;
                sendbuf[2] = 0;
                sendbuf[3] = 0xEB;
                sendbuf[4] = 0x14;
                sendbuf[5] = 0;
                sendbuf[6] = 0;
                ST_BYTE bySum = 0x00;
                for(int i = 0; i < 12; ++i)
                bySum += sendbuf[i];
                sendbuf[7] = bySum;
                sendbuf[8] = 0x16;
                this->Send(sendbuf,9);
}

void          Cchangyang::AdjustTime()
{
                time_t rawtime;
                struct tm *ptminfo;
                time(&rawtime);
                ptminfo = localtime(&rawtime);


                ST_BYTE sendbuf[32];
                sendbuf[0] = 0xEB;
                sendbuf[1] = 0x09;//
                sendbuf[2] = 0;
                sendbuf[3] = 0xEB;
                sendbuf[4] = 0x0C;
                sendbuf[5] = (ptminfo->tm_year + 1900)>>8;
                sendbuf[6] = ptminfo->tm_year + 1900;
                sendbuf[7] = ptminfo->tm_mon + 1;
                sendbuf[8] = ptminfo->tm_mday;
                sendbuf[9] = ptminfo->tm_hour;
                sendbuf[10] = ptminfo->tm_min;
                sendbuf[11] = ((ptminfo->tm_sec)*1000)>>8;
                sendbuf[12] = (ptminfo->tm_sec)*1000;
                ST_BYTE bySum = 0x00;
                for(int i = 0; i < 12; ++i)
                bySum += sendbuf[i];
                sendbuf[13] = bySum;
                sendbuf[14] = 0x16;
                this->Send(sendbuf,15);
}


