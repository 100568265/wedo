#include "C104.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"
#include <time.h>
#include <iostream>
#include <unistd.h>
C104::C104()
{
    //ctor
}

C104::~C104()
{
    //dtor
}

unsigned long GetTickCount()
{
  struct timeval tv;
  if( gettimeofday(&tv, NULL) != 0 )
    return 0;

  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void	C104::Init()
{
    m_VS = 0;
	m_VR = 0;

	m_timeoutReceive = 30;
	m_nStart = 0;
	nSendIndex = 0;
	m_bCallPH = false;
	m_readlasttime=GetTickCount();
	m_timecallall=GetTickCount();
	m_PHsum = 0;
	nrecnem = 0;
    indexN = 0;
}

void	C104::Uninit()
{

}

C104* CreateInstace()
{
    return new C104();
}



void	C104::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;

    if(! this->GetCurPort ()) {
			return;
    }


    ST_INT	len = this->GetCurPort()->PickBytes(pbuf,6,1000);
    if(len >= 6)
    {
        ST_INT star = 0;
        for(;star < len; star++)
        {
            if(pbuf[star] == 0x68)
                return;
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
        len = this->GetCurPort()->PickBytes(pbuf,6,1000);
        ST_INT ndatalen=pbuf[1]+2;
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
    }
    else
    {
        ShowMessage ("Insufficient data length");
        return;
    }
    if (! this->IsOpened()) {
        m_nStart = 0;
        return;
    }
    /*
	if(this->GetCurPort())
	{
		ST_INT	len = this->GetCurPort()->PickBytes(pbuf,6,1000);
		if(len >= 6)
		{
			ST_INT star = 0;
			for(;star < len; star++)
			{
				if(pbuf[star] == 0x68)
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
			len = this->GetCurPort()->PickBytes(pbuf,6,1000);
			ST_INT ndatalen=pbuf[1]+2;
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
		}
		else
		{
            ShowMessage ("Insufficient data length");
		}
//		if (! this->GetCurPort()->IsOpened())
//			m_nStart = 0;
	}
	else {
		m_nStart = 0;
	}*/
}

ST_BOOLEAN	C104::OnSend()
{

 //   if (this->GetCurPort())
 //       this->GetCurPort()->Clear();

	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)
			{
                ShowMessage("遥控选择");
				SendPreYK(m_curTask.taskAddr+0x6000,m_curTask.taskValue);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 1)
			{
                SendYK(m_curTask.taskAddr+0x6000,m_curTask.taskValue);
				ShowMessage("遥控执行");
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 2)
            {
                m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                memset(&m_curTask,0,sizeof(m_curTask));
				return false;
            }
			m_bTask = true;
		}
    }

//       this->GetDevice()->GetChannel()->GetChannelInfo()->
//        m_b0701 = atoi(this->GetCurPort()->m_pCChannel->m_channelParam.channeladdr)%10;
        m_b0701 = 0x01;
		DWORD nowt = GetTickCount();
		DWORD waittime = abs(long(nowt-m_readlasttime))/1000;
		DWORD calltime = abs(long(nowt-m_timecallall)) /1000;
		if (waittime > m_timeoutReceive * 0.5)
		{
			ST_BYTE sendbuf[] = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
			this->Send (sendbuf, 6);
		}
		if (waittime > m_timeoutReceive)
			m_nStart = 0;
		if (calltime > 30*60 && m_nStart)
		{
			ShowMessage("总召");
			this->GetCurPort()->Clear();
			SendTime();
			STARTDT();  // 激活传输启动
			sleep(500);
			CallAll();  //总召 召唤YC，YX
			m_bCallPH = false;
			m_timecallall = GetTickCount();
		}

		else if(m_nStart == 0)
		{
			sleep(1000);
			STARTDT();
		}

    return true;
}

ST_BOOLEAN	C104::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    m_readlasttime = GetTickCount();
	m_nStart = 1;

	m_pcl.TESTFR=true;

	if(!m_bCallPH || m_PHsum<10)
	{
		sleep(100);
		m_PHsum++;
	}

	nrecnem++;
	if (nrecnem>115)
		nrecnem = 0;
    if(len == 0x06)
	{
		if((pbuf[1] == 0x04)&&((pbuf[2] == 0x07)||(pbuf[2] == 0x0B)))
		{
			m_VS = m_VR = 0;
			CallAll();
			m_bCallPH = false;
		}
		if ((pbuf[1] == 0x04) && pbuf[2] == 0x43)
		{
			ST_BYTE buf[6] = { 0x68, 0x04, 0x83, 0x00, 0x00 };
			this->Send(buf, 6);
		}
		return true;
	}
	ST_BYTE ftype=pbuf[2] & 0x03;
	if(ftype == 0x00 || ftype == 0x02)   // I frame
	{
		m_VR += 2;

		unsigned rcon = m_VR / 2;
		if (!(rcon % 4)) SureFrame();

		ST_BYTE CIDType = pbuf[6];
		switch(CIDType)
		{
			case M_SP_NA_1://单点信息
				{
					if (nrecnem<100)
						Explain_M_SP_NA_1(&pbuf[7],	len-8);
				}
				break;
			case M_SP_TA_1:  //带时标的单点信息
				{
					if (nrecnem<100)
						Explain_M_SP_TA_1(&pbuf[7],	len-8);
				}
				break;
			case M_DP_TA_1:  //带时标的双点信息
				{
					if (nrecnem<100)
						Explain_M_DP_TA_1(&pbuf[7],	len-8);
				}
				break;
			case M_SP_TB_1: //带CP56Time2a时标的单点信息
				{
					Explain_M_SP_TB_1(&pbuf[7],	len-8);
				}
				break;
			case M_DP_TB_1: //带CP56Time2a时标的双点信息
				{
					Explain_M_DP_TB_1(&pbuf[7],	len-8);
				}
				break;
			case M_ME_NA_1:  //测量值，规一化值
				{
					if (nrecnem<100)
						Explain_M_ME_NA_1(&pbuf[7],	len-8);
				}
				break;
			case M_ME_NB_1: //测量值，标度化值
				{
					if (nrecnem<100)
						Explain_M_ME_NB_1(&pbuf[7],	len-8);
				}
				break;
			case M_ME_NC_1: //测量值，短浮点数
				{
					if (nrecnem<100)
						Explain_M_ME_NC_1(&pbuf[7],	len-8);
				}
				break;
			case M_ME_ND_1: //测量值，不带品质描述词的规一化值
				{
					if (nrecnem<100)
						Explain_M_ME_ND_1(&pbuf[7],	len-8);
				}
				break;
			case M_IT_NA_1: //电能脉冲量
				{
					Explain_M_IT_NA_1(&pbuf[7],	len-8);
				}
				break;
			case M_IT_TB_1:
				{
					if (nrecnem<100)
						Explain_M_IT_TB_1(&pbuf[7], len - 8);
				} break;
			case C_SC_NA_1: //单点遥控返回
				{
					if(pbuf[8]==0x07 || pbuf[8] == 0x0A)
						this->m_curTask.taskResult.resultCode = 0;
					else
						this->m_curTask.taskResult.resultCode = -1;
					Transfer(&m_curTask);
					memset(&this->m_curTask,0,sizeof(m_curTask));
				}
				break;
			case C_DC_NA_1: //双点遥控返回
				{
					if (pbuf[8] == 0x07 || pbuf[8] == 0x0A)
						this->m_curTask.taskResult.resultCode = 0;
					else
						this->m_curTask.taskResult.resultCode = -1;
					Transfer(&m_curTask);
					memset(&this->m_curTask,0,sizeof(m_curTask));
				}
			case 0x64:
				{
					if(pbuf[8]==0x0a)
					{
						if(!m_bCallPH)
						{
//							this->OnShowMsg("总召电度",0);
							CallPH();
							m_PHsum =0;
							m_bCallPH = true;
						}
					}
				}
			default:
				break;
		}
		ClearClientLink();
	}
	if(ftype == 0x01 )                   // S frame
	{
		//确认帧 先忽略
	}
	if(ftype == 0x03 )                   // U frame
	{
		ClearClientLink();
	}
    return true;
}

ST_BOOLEAN	C104::IsSupportEngine(ST_INT engineType)
{
    return engineType == EngineBase::ENGINE_FULLING;
  //  return 1;
}

void C104::SureFrame()
{
	ST_BYTE pbuf[6];
	pbuf[0] = 0x68;
	pbuf[1] = 0x04;
	pbuf[2] = 0x01;             //00000001 表示S格式
	pbuf[3] = 0x00;
	pbuf[4] = m_VR % 256;
	pbuf[5] = m_VR / 256;
	ST_BOOLEAN sd = this->Send(pbuf,6);
	if(!sd) m_nStart=0;

}

void C104::STARTDT()
{
	ST_BYTE pbuf[6];
	pbuf[0] = 0x68;
	pbuf[1] = 0x04;
	pbuf[2] = 0x07;
	pbuf[3] = 0x00;
	pbuf[4] = 0x00;
	pbuf[5] = 0x00;
	int sd = this->Send(pbuf,6);
	if(sd<=0) m_nStart =0;
}

void C104::ClearClientLink()
{
	m_pcl.First=false;
	m_pcl.Ploy=false;
	m_pcl.STARTDT =false;
	m_pcl.TESTFR=false;
	m_pcl.Len=0;
}


void C104::SendTime()
{

	time_t tt = time(NULL);//获取年月日 时分秒
    struct tm *stm = localtime(&tt);

    struct timeval tv;
    gettimeofday(&tv, NULL); //获取毫秒数


	ST_BYTE sendbuf[20];
	sendbuf[0] = 0x68;
	sendbuf[1] = 0x14;
	sendbuf[2] = m_VS % 256;   //发送序号 2个字节
	sendbuf[3] = m_VS / 256;
	sendbuf[4] = m_VR % 256;   //接收序号 2个字节
	sendbuf[5] = m_VR / 256;
	sendbuf[6] = 0x67;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x06;
	sendbuf[9] = 0x00;
	sendbuf[10] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[11] = (ST_BYTE)((this->GetDevice()->GetDeviceInfo()->Address)>>8);
	sendbuf[12] = 0x00;
	sendbuf[13] = 0x00;
	sendbuf[14] = 0x00;
	sendbuf[15] = tv.tv_usec;
	sendbuf[16] = stm->tm_sec ;  //秒
	sendbuf[17] = stm->tm_min;  //分
	sendbuf[18] = stm->tm_hour;    //时
	sendbuf[19] = stm->tm_mday;     //日
	sendbuf[20] = stm->tm_mon;   //月
	sendbuf[21] = stm->tm_year; //年
	this->Send(sendbuf,22);

}

/*
struct tm
{
  int tm_sec;			 Seconds.	[0-60] (1 leap second)
  int tm_min;			 Minutes.	[0-59]
  int tm_hour;			 Hours.	[0-23]
  int tm_mday;			 Day.		[1-31]
  int tm_mon;			 Month.	[0-11]
  int tm_year;			 Year	- 1900.
  int tm_wday;			 Day of week.	[0-6]
  int tm_yday;			 Days in year.[0-365]
  int tm_isdst;			 DST.		[-1/0/1]
}
*/

//68  0e（APDU长度） 控制字节1 控制字节2 控制字节3 控制字节4
//0f（ASDU） 1（信息体个数）06 00（传送原因） 公共地址低 公共地址高 00 00
//00（信息体地址）14
void C104::CallAll()   //总召 召唤YC，YX
{
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x68;
	sendbuf[1] = 0x0e;
	sendbuf[2] = m_VS % 256;   //发送序号 2个字节
	sendbuf[3] = m_VS / 256;
	sendbuf[4] = m_VR % 256;   //接收序号 2个字节
	sendbuf[5] = m_VR / 256;
	sendbuf[6] = 0x64;         //类型标识
	sendbuf[7] = 0x01;         //可变结构限定词
	sendbuf[8] = 0x06;         //传送原因 2个字节
	sendbuf[9] = 0x00;
	sendbuf[10] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[11] = (ST_BYTE)((this->GetDevice()->GetDeviceInfo()->Address)>>8);
	sendbuf[12] = 0x00;
	sendbuf[13] = 0x00;
	sendbuf[14] = 0x00;
	sendbuf[15] = 0x14;
	ST_BOOLEAN sd = this->Send(sendbuf,16);
	if(!sd) m_nStart=0;
	m_VS += 2;

	ShowMessage("C_IC_NA_1 Order.");
}

//1．主站（客户）端下发单点遥控选择：68 0e（APDU长度） 控制字节1 控制字节2 控
//制字节3 控制字节4  2d（ASDU）1（信息体个数）06 00（传送原因） 公共地址低 公
//共地址地 被控点的3字节信息体地址 1字节的遥控性质
void  C104::SendPreYK(WORD wAddr,ST_BOOLEAN bYkOn)
{
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x68;
	sendbuf[1] = 0x0e;
	sendbuf[2] = m_VS % 256;
	sendbuf[3] = m_VS / 256;
	sendbuf[4] = m_VR % 256;
	sendbuf[5] = m_VR / 256;
	sendbuf[6] = 0x2d;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x06;
	sendbuf[9] = 0x00;
	sendbuf[10] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[11] = (ST_BYTE)((this->GetDevice()->GetDeviceInfo()->Address)>>8);
	sendbuf[12] = wAddr;
	sendbuf[13] = wAddr>>8;
	sendbuf[14] = 0x00;
	sendbuf[15] = 0x80|(bYkOn?0x01:0x00);
	ST_BOOLEAN sd = this->Send(sendbuf,16);
	if(!sd) m_nStart=0;
	m_VS += 2;
}

void  C104::SendYK(WORD wAddr,ST_BOOLEAN bYkOn)
{
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x68;
	sendbuf[1] = 0x0e;
	sendbuf[2] = m_VS % 256;
	sendbuf[3] = m_VS / 256;
	sendbuf[4] = m_VR % 256;
	sendbuf[5] = m_VR / 256;
	sendbuf[6] = 0x2d;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x06;
	sendbuf[9] = 0x00;
	sendbuf[10] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[11] = (ST_BYTE)((this->GetDevice()->GetDeviceInfo()->Address)>>8);
	sendbuf[12] = wAddr;
	sendbuf[13] = wAddr>>8;
	sendbuf[14] = 0x00;
	sendbuf[15] = 0x00|(bYkOn?0x01:0x00);
	ST_BOOLEAN sd =this->Send(sendbuf,16);
	if(!sd) m_nStart=0;
	m_VS += 2;
}

void C104::Explain_M_SP_NA_1(ST_BYTE* pbuf,ST_INT len)  //单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256 + pbuf[7]*256*256 - 0x0001;
		for(int i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i]&0x01;
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[5+i*4] + pbuf[6+i*4]*256 + pbuf[7+i*4]*256*256 - 0x0001;
			ST_BYTE byValue =pbuf[8+i*4]&0x01;
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_SP_TA_1(ST_BYTE* pbuf,ST_INT len)  //带时标的单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256 + pbuf[7]*256*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i*4]&0x01;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[9+i*4]+pbuf[10+i*4]*256;
			ST_INT nMinute = pbuf[11+i*4]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			ST_INT nOutPoint = -1;
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s(%d) 事件 %s "
				: "%04d-%02d-%02d %02d:%02d:%02d %03d ms %s(%d) Event %s "),
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName, nIndexYX, byValue ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[5+i*7] + pbuf[6+i*7]*256 + pbuf[7+i*7]*256*256 - 0x0001;
			ST_BYTE byValue =pbuf[8+i*7]&0x01;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[9+i*7]+pbuf[10+i*7]*256;
			ST_INT nMinute = pbuf[11+i*7]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				: "%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName, nIndexYX, byValue ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_DP_NA_1(ST_BYTE* pbuf,ST_INT len)  //双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[5+i*4] + pbuf[6+i*4]*256 + pbuf[7+i*4]*256*256- 0x0001;
			ST_BYTE byValue =pbuf[8+i*4]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_DP_TA_1(ST_BYTE* pbuf,ST_INT len)  //带时标的双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0001;
		for(int i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i*4]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			int nMillisecond = pbuf[9+i*4]+pbuf[10+i*4]*256;
			int nMinute = pbuf[11+i*4]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			int nOutPoint = -1;
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName, nIndexYX, (byValue == 1) ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		int nYXNum = pbuf[0]&0x7f;
		for(int i=0;i<nYXNum;i++)
		{
			int nIndexYX =  pbuf[5+i*7] + pbuf[6+i*7]*256 + pbuf[7+i*7]*256*256- 0x0001;
			ST_BYTE byValue =pbuf[8+i*7]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			int nMillisecond = pbuf[9+i*7]+pbuf[10+i*7]*256;
			int nMinute = pbuf[11+i*7]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName, nIndexYX, (byValue == 1) ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_SP_TB_1(ST_BYTE* pbuf,ST_INT len) //带CP56Time2a时标的单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256 +pbuf[7]*256*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i*8]&0x01;
			/*
			/////SOE//////////////////
			CP56Time2a st;
			st.Millisecond = pbuf[9+i*8]+pbuf[10+i*8]*256;
			st.Minute = pbuf[11+i*8]&0x3f;
			st.Hour = pbuf[12+i*8]&0x1f;
			st.Day = pbuf[13+i*8]&0x1f;
			st.Month = pbuf[14+i*8]&0x0f;
			st.Year = 2000 + (pbuf[15 + i * 8] & 0x7f);
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					st.Year,st.Month,st.Day,st.Hour,st.Minute,st.Millisecond/1000,st.Millisecond%1000,
					chSoeName, nIndexYX, byValue ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		int nYXNum = pbuf[0]&0x7f;
		for(int i=0;i<nYXNum;i++)
		{
			int nIndexYX =  pbuf[5+i*11] + pbuf[6+i*11]*256 + pbuf[7+i*11]*256*256- 0x0001;
			ST_BYTE byValue =pbuf[8+i*11]&0x01;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.Millisecond = pbuf[9+i*11]+pbuf[10+i*11]*256;
			st.Minute = pbuf[11+i*11]&0x3f;
			st.Hour = pbuf[12+i*11]&0x1f;
			st.Day = pbuf[13+i*11]&0x1f;
			st.Month = pbuf[14+i*11]&0x0f;
			st.Year = 2000 +(pbuf[15+i*11] & 0x7f);
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					st.Year,st.Month,st.Day,st.Hour,st.Minute,st.Millisecond/1000,st.Millisecond%1000,
					chSoeName, nIndexYX, byValue ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_DP_TB_1(ST_BYTE* pbuf,ST_INT len) //带CP56Time2a时标的双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[8+i*8]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.Millisecond = pbuf[9+i*8]+pbuf[10+i*8]*256;
			st.Minute = pbuf[11+i*8]&0x3f;
			st.Hour = pbuf[12+i*8]&0x1f;
			st.Day = pbuf[13+i*8]&0x1f;
			st.Month = pbuf[14+i*8]&0x0f;
			st.Year = 2000 + (pbuf[15 + i * 8] & 0x7f);
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					st.Year,st.Month,st.Day,st.Hour,st.Minute,st.Millisecond/1000,st.Millisecond%1000,
					chSoeName, nIndexYX, (byValue == 1) ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			int nIndexYX =  pbuf[5+i*11] + pbuf[6+i*11]*256 + pbuf[7+i*11]*256*256- 0x0001;
			ST_BYTE byValue =pbuf[8+i*11]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.Millisecond = pbuf[9+i*11]+pbuf[10+i*11]*256;
			st.Minute = pbuf[11+i*11]&0x3f;
			st.Hour = pbuf[12+i*11]&0x1f;
			st.Day = pbuf[13+i*11]&0x1f;
			st.Month = pbuf[14+i*11]&0x0f;
			st.Year = 2000 + (pbuf[15 + i * 11] & 0x7f);
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format((CLocale::SysLangIsChinese()?"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)事件 %s "
				:"%04d-%02d-%02d %02d:%02d:%02d %03d ms %s (%d)event %s "),
					st.Year,st.Month,st.Day,st.Hour,st.Minute,st.Millisecond/1000,st.Millisecond%1000,
					chSoeName, nIndexYX, (byValue == 1) ? TurnOnStr : TurnOffStr);
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void C104::Explain_M_ME_NA_1(ST_BYTE* pbuf,ST_INT len)  //测量值，规一化值
{
	if(pbuf[0]&0x80) //顺序
	{
		int nYCNum = pbuf[0]&0x7f;
		int nIndexYC = 0;
		if(m_b0701)
			nIndexYC = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0701 + 10000;
		else
			nIndexYC =pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x4001 + 10000;
		for(int i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[9+i*3]&0x80;
			float fValue  = 0.0;
			if(bSign)
			{
				fValue = -1*((0xffff - pbuf[8+i*3] - pbuf[9+i*3]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				fValue = pbuf[8+i*3] + pbuf[9+i*3]*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		int nYCNum = pbuf[0]&0x7f;
		for(int i=0;i<nYCNum;i++)
		{
			int nIndexYC = 0;
			if(m_b0701)
				nIndexYC = pbuf[5+i*6] + pbuf[6+i*6]*256 + pbuf[7+i*6]*256*256 - 0x0701 + 10000;
			else
				nIndexYC =pbuf[5+i*6] + pbuf[6+i*6]*256 + pbuf[7+i*6]*256*256 - 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[9+i*6]&0x80;
			float fValue = 0.0;
			if(bSign)
			{
				fValue = -1*((0xffff - pbuf[8+i*6] - pbuf[9+i*6]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				fValue= pbuf[8+i*6] + (pbuf[9+i*6]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			if((nIndexYC-10000) == 195)
			{
				ShowMessage("/////////////////////////");
			}
		}
	}
}

void C104::Explain_M_ME_NB_1(ST_BYTE* pbuf,ST_INT len) //测量值，标度化值
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
		if(m_b0701)
			nIndexYC = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0701 + 10000;
		else
			nIndexYC =pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[9+i*3]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[8+i*3] - pbuf[9+i*3]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				float fValue = pbuf[8+i*3] + (pbuf[9+i*3]&0x7f)*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		int nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
			if(m_b0701)
				nIndexYC = pbuf[5+i*6] + pbuf[6+i*6]*256 + pbuf[7+i*6]*256*256 - 0x0701 + 10000;
			else
				nIndexYC =pbuf[5+i*6] + pbuf[6+i*6]*256 + pbuf[7+i*6]*256*256 - 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[9+i*6]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[8+i*6] - pbuf[9+i*6]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				float fValue = pbuf[8+i*6] + (pbuf[9+i*6]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
		}
	}
}

void C104::Explain_M_ME_NC_1(ST_BYTE* pbuf,ST_INT len) //测量值，短浮点数
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
		if(m_b0701)
			nIndexYC = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x0701 + 10000;
		else
			nIndexYC =pbuf[5]+pbuf[6]*256+pbuf[7]*256*256 - 0x4001 + 10000;
		for(ST_INT i = 0; i < nYCNum; i++)
		{
			DWORD value = pbuf[8+i*5] + pbuf[9+i*5]*256+ pbuf[10+i*5]*256*256+ pbuf[11+i*5]*256*256*256;
			float fValue = (float&)(*((short*)&value));
			this->UpdateValue(nIndexYC++,(float)fValue);
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
			if(m_b0701)
				nIndexYC = pbuf[5+i*8] + pbuf[6+i*8]*256 + pbuf[7+i*8]*256*256 - 0x0701 + 10000;
			else
				nIndexYC =pbuf[5+i*8] + pbuf[6+i*8]*256 + pbuf[7+i*8]*256*256 - 0x4001 + 10000;
			DWORD value = pbuf[8+i*8] + pbuf[9+i*8]*256 + pbuf[10+i*8]*256*256 + pbuf[11+i*8]*256*256*256;
			float fValue = (float&)(*((short*)&value));
			this->UpdateValue(nIndexYC,(float)fValue);

		}
	}
}

void C104::Explain_M_ME_ND_1(ST_BYTE* pbuf,ST_INT len) //测量值，不带品质描述词的规一化值
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
		if(m_b0701)
			nIndexYC = pbuf[5]+pbuf[6]*256 +pbuf[7]*256*256 - 0x0701 + 10000;
		else
			nIndexYC =pbuf[5]+pbuf[6]*256 +pbuf[7]*256*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[9+i*2]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[8+i*2] - pbuf[9+i*2]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				float fValue = pbuf[8+i*2] + (pbuf[9+i*2]&0x7f)*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		int nYCNum = pbuf[0]&0x7f;
		for(int i=0;i<nYCNum;i++)
		{
			int nIndexYC = 0;
			if(m_b0701)
				nIndexYC = pbuf[5+i*5] + pbuf[6+i*5]*256 + pbuf[7+i*5]*256*256- 0x0701 + 10000;
			else
				nIndexYC =pbuf[5+i*5] + pbuf[6+i*5]*256 + pbuf[7+i*5]*256*256- 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[9+i*5]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[8+i*5] - pbuf[9+i*5]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				float fValue = pbuf[8+i*5] + (pbuf[9+i*5]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
		}
	}
}

void C104::Explain_M_IT_NA_1(ST_BYTE* pbuf,ST_INT len) //电能脉冲量
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = pbuf[5]+pbuf[6]*256+pbuf[7]*256*256- 0x6401 + 20000;
		for(int i=0;i<nYCNum;i++)
		{
			double dbValue = pbuf[8+i*5] + pbuf[9+i*5]*256 + pbuf[10+i*5]*256*256 + pbuf[11+i*5]*256*256*256;
			this->UpdateValue(nIndexYC++,(double)dbValue);
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(int i=0;i<nYCNum;i++)
		{
			int nIndexYC =  pbuf[5+i*8]+pbuf[6+i*8]*256+pbuf[7+i*8]*256*256 - 0x6401 + 20000;
			double dbValue = pbuf[8+i*8] + pbuf[9+i*8]*256 + pbuf[10+i*8]*256*256 + pbuf[11+i*8]*256*256*256;
			this->UpdateValue(nIndexYC,(double)dbValue);
		}
	}
}

void C104::Explain_M_IT_TB_1(ST_BYTE*pbuf, ST_INT len)
{
	if (pbuf[0] & 0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0] & 0x7f;
		ST_INT nIndexYC = pbuf[5] + pbuf[6] * 256 + pbuf[7] * 256 * 256 - 0x6401 + 20000;
		for (ST_INT i = 0; i<nYCNum; i++)
		{
			double dbValue = pbuf[8 + i * 12] + pbuf[9 + i * 12] * 256 + pbuf[10 + i * 12] * 256 * 256 + pbuf[11 + i * 12] * 256 * 256 * 256;
			this->UpdateValue(nIndexYC++, (double)dbValue);
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0] & 0x7f;
		for (ST_INT i = 0; i<nYCNum; i++)
		{
			ST_INT   nIndexYC = pbuf[5 + i * 15] + pbuf[6 + i * 15] * 256 + pbuf[ 7 + i * 15] * 256 * 256 - 0x6401 + 20000;
			double dbValue = pbuf[8 + i * 15] + pbuf[9 + i * 15] * 256 + pbuf[10 + i * 15] * 256 * 256 + pbuf[11 + i * 15] * 256 * 256 * 256;
			this->UpdateValue(nIndexYC, (double)dbValue);
		}
	}
}




