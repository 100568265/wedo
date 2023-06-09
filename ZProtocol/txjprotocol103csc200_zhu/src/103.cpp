
#include "103.h"
#include "Channel.h"
#include "datetime.h"
#include <stdio.h>

C103::C103()
{
    time(&nT);
    time(&oT);
}

C103::~C103()
{
}

void	C103::Init()
{
	SendState = 1;
	FCB = 0;
	Newcurtime=clock();
	oldcurtime=clock();
	sendserchtime=clock();
	CLtime=sendserchtime;
	BreakCallState=0;
	sendflag=0;

	m_bTask=false;
}

void	C103::Uninit()
{

}

C103* CreateInstace()
{
    return  new C103();
}

ST_BOOLEAN	C103::IsSupportEngine(ST_INT engineType)
{
	return 1;
}

void	C103::OnRead(ST_BYTE* pbuf, int& readed)
{
	if(this->GetCurPort())
	{
		int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		lineInterval = (lineInterval > 500 ? lineInterval : 500);

		int	len = this->GetCurPort()->PickBytes(pbuf, 4, lineInterval);
		if(len >= 4)
		{
			int star = 0;
			for(; star < len; star++)
			{
				if(pbuf[star] == 0x10 || pbuf[star] ==0x68)
					break;
			}
			if(star > 0)
			{
				//star大于0，说明有乱码， 把之前的乱码丢掉
				this->GetCurPort()->ReadBytes(pbuf,star);
			}
			if(star == len)
			{
				//全是乱码
				this->GetCurPort()->Clear();//清空乱码
				return;
			}
			if (pbuf[0]==0x10 && pbuf[2]==(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) //收到的是固定帧
			{
				this->GetCurPort()->ReadBytes(pbuf,5,3000);
				readed=5;
				return;
			}
			else if(pbuf[0]==0x10 && pbuf[2]!=(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
            {
                this->GetCurPort()->Clear();
            }
			else if (pbuf[0]==0x68 && pbuf[5]==(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)//收到的是可变帧
			{
				len=pbuf[1]+6;
				this->GetCurPort()->ReadBytes(pbuf,len,3000);
				readed=pbuf[1]+6;
				return;
			}
			else if(pbuf[0]==0x68 && pbuf[5]!=(ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
            {
                this->GetCurPort()->Clear();
            }
			else
			{
				this->GetCurPort()->Clear();//清空乱码
				return;
			}
		}
	}
	else
	{

        Newcurtime=clock();
        if (abs(Newcurtime-oldcurtime)>30 * CLOCKS_PER_SEC) //大于3m
		{
            ShowMessage("try to Restart Link!");
            SendState==0x04;
		}

	}
}

ST_BOOLEAN	C103::OnSend()
{
    Newcurtime=clock();
	ST_BYTE FCV,Code;
	ST_BYTE sendcode=0;

 //   m_bTask = false;
	if (this->HasTask() && this->GetTask(&m_curTask))
	{

		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0) {
				YKSelect(m_curTask);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 1) {
				YKExecut(m_curTask);
				m_bTask = true;
				return true;
			}

		}
		if (m_curTask.taskCmdCode==0x01)//对时
		{
			SendASDU6();
			m_curTask.taskResult.resultCode = 0;
			Transfer (&m_curTask);
			memset(&m_curTask,0,sizeof(m_curTask));
			return false;
		}
	}

    time(&nT);
    if((nT-oT)>10)
    {
        ShowMessage("try to Restart Link!");
        SendState=0x04;
    }

	if (SendState==0x00) //复位
	{
		FCV=0;
		Code=0;
		sendcode=0x40;//PRM
		sendcode=sendcode|(FCB<<5);
		sendcode=sendcode|(FCV<<4);
		sendcode=sendcode|Code;
		SendSetframe(sendcode);
	}
	else if (SendState==0x01)//请求链路状态
	{
		FCV=0;
		Code=0;
		sendcode=0x40;//PRM
		sendcode=sendcode|(FCB<<5);
		sendcode=sendcode|(FCV<<4);
		sendcode=sendcode|Code;
		SendSetframe(sendcode);
		SendState=0x04;
		BreakCallState=1;
		return true;
	}
	else if (SendState==0x02)//召唤一级数据
	{
		FCV=1;
		Code=10;
		sendcode=0x40;//PRM
		sendcode=sendcode|(FCB<<5);
		sendcode=sendcode|(FCV<<4);
		sendcode=sendcode|Code;
		SendSetframe(sendcode);
		return true;
	}
	else if (SendState==0x03)//召唤二级数据
	{
		struct tm now_tm;
		DateTime::localtime (time(0), now_tm);
		newday=now_tm.tm_mday;
		if (newday!=oldday)  //发对时
		{
			oldday=newday;
			if (now_tm.tm_sec==0x00)
			{
				SendASDU6();
				Thread::SLEEP(1000);
				return false;
			}

		}
		if ((abs(Newcurtime-sendserchtime) > 1800 * CLOCKS_PER_SEC)) //大于1800秒就是30分钟
		{
			sendserchtime=clock();
			SendAllSearch();
			return true;
		}
		if (BreakCallState)
		{
			SendAllSearch();
			BreakCallState=0;
			return true;
		}
		if (abs(Newcurtime-oldcurtime)>50 * CLOCKS_PER_SEC) //大于3m
		{
			FCV=0;
			Code=0;
			sendcode=0x40;//PRM
			sendcode=sendcode|(FCB<<5);
			sendcode=sendcode|(FCV<<4);
			sendcode=sendcode|Code;
			SendSetframe(sendcode);
			SendState=0x04;
			BreakCallState=1;
			return true;
		}

		if (abs(Newcurtime-CLtime)>7 * CLOCKS_PER_SEC)//发送测量数据
		{
			CLtime=Newcurtime;
			SendASDU21();
			return true;
		}

		FCV=1;
		Code=11;
		sendcode=0x40;//PRM
		sendcode=sendcode|(FCB<<5);
		sendcode=sendcode|(FCV<<4);
		sendcode=sendcode|Code;
		SendSetframe(sendcode);
		return true;
	}
	else if (SendState==0x04)//发送总查询
	{
		SendAllSearch();
		return true;
	}
	return true;
}
ST_BOOLEAN	C103::OnProcess(ST_BYTE* pbuf,int len)
{
	oldcurtime=clock();
	time(&oT);
	ST_BYTE DFC,ACD,Recode;

	FCB = !FCB;

	if (pbuf[0]==0x10)//固定帧
	{
		Recode=pbuf[1]&0xf;
		DFC=(pbuf[1]&0x10)>>4;
		ACD=(pbuf[1]&0x20)>>5;
		if (ACD)
		{
			SendState=2;  //ACD为1代表要召唤一级数据
			return true;
		}
		else
		{
			if (BreakCallState)
			{
				BreakCallState=0;
				SendState=4;
				return true;
			}
			SendState=3;  //ACD为1代表要召唤二级数据
		}
		if(Recode==0x00)//确认
		{

		}
		else if (Recode==0x01)//链路忙
		{
		}
		else if (Recode==0x09)//从站没有所召唤的数据
		{
			SendState=3; //召唤二级数据
		}
		else if (Recode==11)//从站以链路状态响应主站请求
		{
		}
	}
	else if (pbuf[0]==0x68)//可变帧
	{
		ST_BYTE typ;
		typ=pbuf[6];
		ACD=(pbuf[4]&0x20)>>5;
		switch (typ)
		{
		case 0x32://遥测
			{
				EXpainYc(pbuf);
				break;
			}
		case 0x2c://全遥信
			{
				EXpainYx(pbuf);
				break;
			}
		case 0x28://变位遥信
			{
				EXpainBwYx(pbuf);
				break;
			}
		case 0x29://SOE
			{
				EXpainSOE(pbuf);
				break;
			}
		case 0x0a:
			{
			    if(m_bTask)
			    {
			        ShowMessage("Enter YK group");
			      //  char buffer[64];
			       // sprintf(buffer,"code 8 byte :%d",pbuf[8]);
			        //ShowMessage(buffer);
			        if(pbuf[8]==0x2C) //
                    {
                       // if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
                        {
                            ShowMessage("YK successful");
                            m_curTask.taskResult.resultCode = 0;
                            m_curTask.isTransfer = 1;
                            Transfer(&m_curTask);
                            memset(&m_curTask, 0, sizeof(m_curTask));

                        }

                    }
                    else if(pbuf[8]==0x28)
                    {
                     //   if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
                        {
                            ShowMessage("YK successful");
                            m_curTask.taskResult.resultCode = 0;
                            m_curTask.isTransfer = 1;
                            Transfer(&m_curTask);
                            memset(&m_curTask, 0, sizeof(m_curTask));
                        }

                    }
			        else if(pbuf[8]==0x29)
                    {
                        ShowMessage("device YK failure!");
                    }
                    m_bTask = false;
			    }
                else
                {
                    ASDU10(pbuf);//解遥测
                }

                break;
			}
		case 0x2A:
			{
				ASDU2a(pbuf);//压板和遥信
				break;
			}
		case 0x01:
			{
				ASDU1(pbuf);//告警
				break;
			}
		case 0x02:
			{
				ASDU2(pbuf);//保护动作信息
				break;
			}
		}
		if (ACD)
		{
			SendState=2;  //召唤一级
		}
		else
		{
			SendState=3; //召唤二级
		}
	}

	return true;
}


void	C103::SendSetframe(ST_BYTE code) //发送固定帧
{
	ST_BYTE sendbuf[7] = {0};
	sendbuf[0]=0x10;
	sendbuf[1]=code;
	sendbuf[2]=this->GetDevice()->GetDeviceInfo()->Address;
	ST_BYTE CS=0;
	CS=(sendbuf[1]+sendbuf[2])%256;
	sendbuf[3]=CS;
	sendbuf[4]=0x16;
	this->Send(sendbuf,5);
}
void    C103::EXpainYc(ST_BYTE* Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[7]&0x7f;
	float fvalue=0;
	for (int i=0;i<yccount;i++)
	{
		funit=(Rebuf[12]&0x80)>>7;
		fvalue=((Rebuf[12+2*i]*256+Rebuf[13]+2*i)>>3)&0x1fff;
		if (funit)
		{
			fvalue=-1*fvalue;
		}
		this->UpdateValue(10000+i,fvalue);
	}
}
void    C103::EXpainYx(ST_BYTE* Rebuf)
{
	ST_BYTE yxcount,funit;
	ST_BYTE bvalue;
	yxcount=Rebuf[7]&0x7f;
	for (int k=0;k<yxcount;k++)
	{
		for (int i=0;i<8;i++)
		{
			bvalue=(Rebuf[12+k]>>i)&0x01;
			this->UpdateValue(i+8*k,bvalue);
		}
	}
}
void    C103::EXpainBwYx(ST_BYTE* Rebuf)
{

}
void    C103::EXpainSOE(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	inf=Rebuf[11]-110;
	ST_BYTE bvalue;
	bvalue=Rebuf[12];
	int nhour,nminte,nmillsecond;
	nhour=Rebuf[16];
	nminte=Rebuf[15];
	nmillsecond=Rebuf[13]+Rebuf[14]*256;
	// CString ss="";
	// CString Pointname="";
	// CString devicename="";
	// Pointname=this->GetCurLine()->m_pDataAreas[1].items[inf].itemName;
	// devicename=this->GetCurLine()->m_lineParam.lineName;
	// Pointname=devicename+Pointname;
	//Pointname.Format("%s的%s",this->GetCurLine()->m_lineParam.lineName,this->GetCurLine.m_pDataAreas[1].items[inf]->itemName);
	// ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
	// Pointname=Pointname+ss;
	// this->ReportEvent(Pointname,"soebj");
	TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
}

void     C103::SendAllSearch()
{
	ST_BYTE sendcode,Code,FCV;
	FCV=1;
	Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
     ST_BYTE sendbuf[20] = {0};
	 sendbuf[0]=0x68;
	 sendbuf[1]=0x09;
	 sendbuf[2]=0x09;
	 sendbuf[3]=0x68;
	 sendbuf[4]=sendcode;
	 sendbuf[5]=this->GetDevice()->GetDeviceInfo()->Address;
	 sendbuf[6]=0x07; //adsu
	 sendbuf[7]=0x81; //vsq
	 sendbuf[8]=0x09;
	 sendbuf[9]=this->GetDevice()->GetDeviceInfo()->Address;
	 sendbuf[10]=0xff; //fun
	 sendbuf[11]=0x00;  //inf
	 sendbuf[12]=0x00;  //序号
	 ST_BYTE cssum=0;
	 for (int i=4;i<13;i++)
	 {
		cssum=cssum+sendbuf[i];
	 }
	 sendbuf[13]=cssum;
	 sendbuf[14]=0x16;
	 this->Send(sendbuf,15);
}

void	C103::SendASDU21()
{

	ST_BYTE sendcode,Code,FCV;
	FCV=1;
	Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE sendbuf[20] = {0};
	sendbuf[0]=0x68;
	sendbuf[1]=0x0d;
	sendbuf[2]=0x0d;
	sendbuf[3]=0x68;
	sendbuf[4]=sendcode;
	sendbuf[5]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6]=0x15; //adsu
	sendbuf[7]=0x81; //vsq
	sendbuf[8]=0x2a;
	sendbuf[9]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10]=0xfe; //fun
	sendbuf[11]=0xf1;  //inf //读遥测
	sendbuf[12]=0x00;  //Rii
	sendbuf[13]=0x01;  //NOG
	if (this->GetDevice()->GetDeviceInfo()->DataAreas && this->GetDevice()->GetDeviceInfo()->DataAreasCount > 2)
		sendbuf[14]=this->GetDevice()->GetDeviceInfo()->DataAreas[2].readCode;  //GIN 09 为保护数据， 0d为测量数据
	sendbuf[15]=0x00;  //GIN
	sendbuf[16]=0x01;  //KOD
	ST_BYTE cssum=0;
	for (int i=4;i<17;i++)
	{
		cssum=cssum+sendbuf[i];
	}
	sendbuf[17]=cssum;
	sendbuf[18]=0x16;
	 this->Send(sendbuf,19);
}

void    C103::ASDU2a(ST_BYTE* Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[7]&0x7f;
	ST_BYTE fvalue=0;

	for (int i=0;i<yccount;i++)
	{
		fvalue= (Rebuf[12+i] == 0x02)?0x01:0x00;
		if (Rebuf[11]==0x20) //压板
		{
			this->UpdateValue(i,(ST_BYTE)fvalue);
		}
		else if (Rebuf[11]==0x40)//开入遥信
		{
			this->UpdateValue(i+10,(ST_BYTE)fvalue);
		}
	}

}

void	C103::ASDU10(ST_BYTE* Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[13]&0x7f;
	float fvalue=0;
	for (int i=0;i<yccount;i++)
	{
		if (Rebuf[14+i*10]==0x09) //保护测量
		{
			char bvalue[4];
			bvalue[0]=Rebuf[i*10+20];
			bvalue[1]=Rebuf[i*10+1+20];
			bvalue[2]=Rebuf[i*10+2+20];
			bvalue[3]=Rebuf[i*10+3+20];
			fvalue =*(float*)(bvalue);
			this->UpdateValue(10000+i,fvalue);
		}
		else if (Rebuf[14+i*10]==0x0d) //测量数据
		{
			char bvalue[4];
			bvalue[0]=Rebuf[i*10+20];
			bvalue[1]=Rebuf[i*10+1+20];
			bvalue[2]=Rebuf[i*10+2+20];
			bvalue[3]=Rebuf[i*10+3+20];
			fvalue =*(float*)(bvalue);
			this->UpdateValue(10000+i,fvalue);
		}
		else if (Rebuf[14+i*10]==0x0E) //电度数据
		{
			char bvalue[4];
			bvalue[0]=Rebuf[i*10+20];
			bvalue[1]=Rebuf[i*10+1+20];

			bvalue[2]=Rebuf[i*10+2+20];
			bvalue[3]=Rebuf[i*10+3+20];
			fvalue =*(float*)(bvalue);
			this->UpdateValue(20000+i,fvalue);
		}
		//fvalue=((Rebuf[12+2*i]*256+Rebuf[13]+2*i)>>3)&0x1fff;

	}
}


void	C103::ASDU1(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	if (Rebuf[11]>=176 &&Rebuf[11]<=239 ) //SOE
	{
		inf=abs(Rebuf[11]-176);
		ST_BYTE bvalue;
		bvalue= Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;
/*		CString ss="";
		CString Pointname="";
		CString devicename="";
		Pointname=this->GetCurLine()->m_pDataAreas[1].items[inf].itemName;
		devicename=this->GetCurLine()->m_lineParam.lineName;
		Pointname=devicename+Pointname;*/
/*		ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
		Pointname=Pointname+ss;
		this->ReportEvent(Pointname,"soebj");*/
		TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
	}
	else if (Rebuf[11]>=32 &&Rebuf[11]<=41 ) //压板
	{
		this->UpdateValue(Rebuf[11]-32,Rebuf[12]==0x02?0x01:0x00);
		inf=abs(Rebuf[11]-32);
		ST_BYTE bvalue;
		bvalue= Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;
/*		CString ss="";
		CString Pointname="";
		CString devicename="";
		Pointname=this->GetCurLine()->m_pDataAreas[0].items[inf].itemName;
		devicename=this->GetCurLine()->m_lineParam.lineName;
		Pointname=devicename+Pointname;
		ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
		Pointname=Pointname+ss;
		this->ReportEvent(Pointname,"soebj");*/
		TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
	}
	else if (Rebuf[11]>=64 &&Rebuf[11]<=92 ) //开入
	{
		inf=abs(Rebuf[11]-64+10);
		ST_BYTE bvalue;
		bvalue=Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;
/*		CString ss="";
		CString Pointname="";
		CString devicename="";
		Pointname=this->GetCurLine()->m_pDataAreas[0].items[inf].itemName;
		devicename=this->GetCurLine()->m_lineParam.lineName;
		Pointname=devicename+Pointname;
		ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
		Pointname=Pointname+ss;
		this->ReportEvent(Pointname,"soebj");*/
		this->UpdateValue(Rebuf[11]-64+10,Rebuf[12]==0x02?0x01:0x00);
		TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
	}
}

void	C103::ASDU2(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	if (Rebuf[11]>=203 &&Rebuf[11]<=239 ) //SOE
	{
		inf=abs(Rebuf[11]-176);
		ST_BYTE bvalue;
		bvalue=(Rebuf[12] == 0x02) ? 0x01:0x00;
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16+4]&0x7f;
		nminte=Rebuf[15+4]&0x1f;
		nmillsecond=Rebuf[13+4]+Rebuf[14+4]*256;
/*		CString ss="";
		CString Pointname="";
		CString devicename="";
		Pointname=this->GetCurLine()->m_pDataAreas[1].items[inf].itemName;
		devicename=this->GetCurLine()->m_lineParam.lineName;
		Pointname=devicename+Pointname;
		ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
		Pointname=Pointname+ss;
		this->ReportEvent(Pointname,"soebj");*/
		TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
	}
}
void C103::SendASDU6()
{

	ST_BYTE sendcode,Code,FCV;
	FCV=1;
	Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE sendbuf[21] = {0};
	sendbuf[0]=0x68;
	sendbuf[1]=0x0f;
	sendbuf[2]=0x0f;
	sendbuf[3]=0x68;
	sendbuf[4]=sendcode;
	sendbuf[5]=0xff;//this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6]=0x06; //adsu
	sendbuf[7]=0x81; //vsq
	sendbuf[8]=0x08;
	sendbuf[9]=0xff;
	sendbuf[10]=0xff; //fun
	sendbuf[11]=0x00;  //inf
	struct tm now_tm;
	DateTime::localtime (time(0), now_tm);
	sendbuf[12]=(now_tm.tm_sec * 1000)%256; //MS
	sendbuf[13]=(now_tm.tm_sec * 1000)/256; //ms
	sendbuf[14]=now_tm.tm_min;  //min
	sendbuf[15]=now_tm.tm_hour; //hour
	sendbuf[16]=now_tm.tm_mday; //day
	sendbuf[17]=now_tm.tm_mon + 1;  //mon
	sendbuf[18]=now_tm.tm_year - 100;  //year
	ST_BYTE cssum=0;
	for (int i=4;i<19;i++)
	{
		cssum=cssum+sendbuf[i];
	}
	sendbuf[19]=cssum;
	sendbuf[20]=0x16;
	 this->Send(sendbuf,21);
}

void    C103::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
{
	struct tm t_tm;
	DateTime::localtime (time(0), t_tm);

	ProtocolTask task;
	Strcpy(task.taskCmd,"SOE");
	task.isTransfer     = true;
	task.transChannelId = -1;
	task.channelId      = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
	task.deviceId       = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.taskCmdCode    = 0;
	task.taskParamLen   = 14;
	task.taskAddr       = addr;
	task.taskValue      = statu;
	task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.ignoreBack     = 1;
	task.taskTime       = 1000;
	task.taskParam[0]   = (t_tm.tm_year + 1900) % 256;
	task.taskParam[1]   = (t_tm.tm_year + 1900) / 256;
	task.taskParam[2]   =  t_tm.tm_mon  + 1;
	task.taskParam[3]   =  t_tm.tm_mday;
	task.taskParam[4]   =  hour;
	task.taskParam[5]   =  min;
	task.taskParam[6]   =  msec / 1000;
	task.taskParam[7]   = (msec % 1000) % 256;
	task.taskParam[8]   = (msec % 1000) / 256;
	task.taskParam[9]   = statu;
	task.taskParam[10]  = addr % 256;
	task.taskParam[11]  = addr / 256;
	task.taskParam[12]  = task.taskAddr1 % 256;
	task.taskParam[13]  = task.taskAddr1 / 256;
	Transfer(&task);
}

/*
select:
->68 11 11 68 73 32 0A(1) 81 28 32 FE(2) F9(3) 35(4) 01(5) 0C 03(6) 01(7) 03 01 01(8) 01(9) CD 16
<-10 20 32 52 16 确认帧
->10 5A 32 8C 16 召唤一级用户数据
<-68 11 11 68 08 01 0A(1) 81 2C(2) 32 FE F9 35 01 0C 03 01 03 01 01 01 35 16
*/
void C103::YKSelect(ProtocolTask & task)
{
    ST_BYTE sendcode,Code,FCV;
	FCV=1;
    Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE data[64] = {0};
	data[ 0] = 0x68;
	data[ 1] = 0x11;
	data[ 2] = 0x11;
	data[ 3] = 0x68;
	data[ 4] = sendcode;
	data[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	data[ 6] = 0x0A;
	data[ 7] = 0x81;
	data[ 8] = 0x28;
	data[ 9] = data[5];
	data[10] = 0xFE;
	data[11] = 0xF9;
	data[12] = 0x35;// RII
	data[13] = 0x01;
	data[14] = 0x0C;
	data[15] = 0x01;//YK fenzu
	data[16] = 0x01;//(7)
	data[17] = 0x03;
	data[18] = 0x01;
	data[19] = 0x01;
	data[20] = (task.taskValue == 2 ? 0x01: 0x00);
//	data[21] = get_check_sum(data);
    char msg[64];
    sprintf(msg,"task value :%d",task.taskValue);
    ShowMessage(msg);
    ST_BYTE cssum=0;
	for (int i=4;i<21;i++)
	{
		cssum=cssum+data[i];
	}
	data[21] = cssum;
	data[22] = 0x16;

	this->Send(data, 23);
}

/*
主：68 11 11 68 73 32 0A(1) 81 28 32 FE(2) FA(3) 36(4) 01(5) 0C 03(6) 01(7) 03 01 01(8) 01(9) CF 16 通
用分类带执行的写条目
（1）类型标识 （2）功能码，通用分类服务 （3）信息序号，带执行的写条目 （ 4）返回信息
标识符 （ 5）通用分类数据集数目 （ 6）通用分类标识序号 （ 7）描述类别 （ 8）通用分类数据描
述 （ 9）通用分类标识数据
子： 10 20 32 52 16 确认帧
主： 10 5A 32 8C 16 召唤一级用户数据
子： 68 0A 0A 68 08 01 0A(1) 81 28(2) 01 FE FA 36 00 EB 16 响应带执行的写条目
*/
void C103::YKExecut(ProtocolTask & task)
{
    ST_BYTE sendcode,Code,FCV;
	FCV=1;
    Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE data[64] = {0};
	data[ 0] = 0x68;
	data[ 1] = 0x11;
	data[ 2] = 0x11;
	data[ 3] = 0x68;
	data[ 4] = sendcode;
	data[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	data[ 6] = 0x0A;
	data[ 7] = 0x81;
	data[ 8] = 0x28;
	data[ 9] = data[5];
	data[10] = 0xFE;
	data[11] = 0xFA;
	data[12] = 0x36;
	data[13] = 0x01;
	data[14] = 0x0C;//通用分类标识序号
	data[15] = 0x01;//
	data[16] = 0x01;
	data[17] = 0x03;//通用分类数据描述
	data[18] = 0x01;
	data[19] = 0x01;
	data[20] = (task.taskValue == 2 ? 0x01: 0x00);
	ST_BYTE cssum=0;
	for (int i=4;i<21;i++)
	{
		cssum=cssum+data[i];
	}
	data[21] = cssum;
//	data[21] = get_check_sum(data);
	data[22] = 0x16;

	this->Send(data, 23);
}
