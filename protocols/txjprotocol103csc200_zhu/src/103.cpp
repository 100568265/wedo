
#include "103.h"
#include "Channel.h"
#include "datetime.h"

C103::C103()
{
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
}

void	C103::Uninit()
{

}

void	C103::OnRead(ST_BYTE* pbuf, int& readed)
{
	if(this->GetCurPort())
	{
		int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		lineInterval = (lineInterval > 3000 ? lineInterval : 3000);//超时时间等于通讯间隔，当超时时间小于三秒时，调整为3s

		int	len = this->GetCurPort()->PickBytes(pbuf, 4, lineInterval);//报文长度len
		if(len >= 4)
		{
			int star = 0;
			for(; star < len; star++)
			{
				if(pbuf[star] == 0x10 || pbuf[star] ==0x68)//固定帧，可变帧
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
			if (pbuf[0]==0x10) //收到的是固定帧
			{
				this->GetCurPort()->ReadBytes(pbuf,5,3000);
				readed=5;//长度
				return;
			}
			else if (pbuf[0]==0x68)//收到的是可变帧
			{
				len=pbuf[1]+6;//数据区长度加6
				this->GetCurPort()->ReadBytes(pbuf,len,3000);
				readed=pbuf[1]+6;
				return;
			}
			else
			{
				this->GetCurPort()->Clear();//清空乱码
				return;
			}
		}
	}
}

ST_BOOLEAN	C103::OnSend()
{
	if (this->HasTask() && this->GetTask(&m_curTask))
	{
		if (m_curTask.taskCmdCode==0x01)//对时
		{
			SendASDU6();
			m_curTask.taskResult.resultCode = 0;
			Transfer (&m_curTask);
			memset(&m_curTask,0,sizeof(m_curTask));
			return false;
		}
	}
	Newcurtime=clock();
	ST_BYTE FCV,Code;
	ST_BYTE sendcode=0;
	if (SendState==0x00) //复位
	{
		FCV=0;//帧计数有效位
		Code=0;//功能码为0
		sendcode=0x40;//PRM，信息传输方向，1为主到从
		sendcode=sendcode|(FCB<<5);//FCB左移五位与
		sendcode=sendcode|(FCV<<4);//FCV左移四位与
		sendcode=sendcode|Code;//功能码与操作
		SendSetframe(sendcode);//发送固定帧报文
	}
	else if (SendState==0x01)//请求链路状态
	{
		FCV=0;//帧计数有效位
		Code=0;//功能码为0
		sendcode=0x40;//PRM，信息传输方向，1为从主到从
		sendcode=sendcode|(FCB<<5);//帧计数位
		sendcode=sendcode|(FCV<<4);//帧计数有效位
		sendcode=sendcode|Code;
		SendSetframe(sendcode);//发送固定帧报文
		SendState=0x04;//总召唤
		BreakCallState=1;
		return true;
	}
	else if (SendState==0x02)//召唤一级数据
	{
		FCV=1;//1验定帧，鉴定FCB为有效
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
	ST_BYTE DFC,ACD,Recode;

	FCB = !FCB;

	if (pbuf[0]==0x10)//固定帧
	{
		Recode=pbuf[1]&0xf;//功能码，控制域后四位
		DFC=(pbuf[1]&0x10)>>4;//数据流控制位，0表示可以接受，1表示已满
		ACD=(pbuf[1]&0x20)>>5;//ACD=1表示有一级数据
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
				  ASDU10(pbuf);//解遥测
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

ST_BOOLEAN	IsSupportEngine(ST_INT engineType)
{
	return 1;
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
//	 CString ss="";
//	 CString Pointname="";
//	 CString devicename="";
//	 Pointname=this->GetCurLine()->m_pDataAreas[1].items[inf].itemName;
//	 devicename=this->GetCurLine()->m_lineParam.lineName;
//	 Pointname=devicename+Pointname;
//	Pointname.Format("%s的%s",this->GetCurLine()->m_lineParam.lineName,this->GetCurLine.m_pDataAreas[1].items[inf]->itemName);
//	 ss.Format("状态为%d,时间为%d点%d分%d ms",bvalue,nhour,nminte,nmillsecond);
//	 Pointname=Pointname+ss;
//	 this->ReportEvent(Pointname,"soebj");
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
		fvalue=Rebuf[12+i];
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
		bvalue=Rebuf[12];
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
		this->UpdateValue(Rebuf[11]-32,Rebuf[12]);
		inf=abs(Rebuf[11]-32);
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
		this->UpdateValue(Rebuf[11]-64+10,Rebuf[12]);
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
		bvalue=Rebuf[12];
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
