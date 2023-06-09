
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
	OneTime=sendserchtime;
	TwoTime=sendserchtime;
	BreakCallState=0;
	sendflag=0;

	m_bTask=false;
}
inline ST_UINT16 bswap16 (ST_UINT16 value)
{
	return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}
inline ST_UINT32 bswap32 (ST_UINT32 value)
{
	return (((ST_UINT32)bswap16(value & 0x0000FFFF) << 16) | bswap16((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 bswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)bswap32(value & 0x00000000FFFFFFFF) << 32) | bswap32((value & 0xFFFFFFFF00000000) >> 32));
}
inline ST_UINT32 wswap32 (ST_UINT32 value)
{
	return (((value & 0x0000FFFF) << 16) | ((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 wswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)wswap32(value & 0x00000000FFFFFFFF) << 32) | wswap32((value & 0xFFFFFFFF00000000) >> 32));
}
void	C103::Uninit()
{

}

C103* CreateInstace()
{
    return  new C103();
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
			if (pbuf[0]==0x10) //收到的是固定帧
			{
				this->GetCurPort()->ReadBytes(pbuf,5,3000);
				readed=5;
				return;
			}
			else if (pbuf[0]==0x68)//收到的是可变帧
			{
				len=pbuf[1]+6;
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
	else
	{
	    /*
        Newcurtime=clock();
        if (abs(Newcurtime-oldcurtime)>30 * CLOCKS_PER_SEC) //大于3m
		{
            ShowMessage("try to Restart Link!");
            SendState==0x01;
		}*/

	}
}

ST_BOOLEAN	C103::OnSend()
{
    Newcurtime=clock();
	ST_BYTE FCV,Code;
	ST_BYTE sendcode=0;

    m_bTask = false;
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
	}

    time(&nT);
    if((nT-oT)>10)
    {
        ShowMessage("try to Restart Link!");
        SendState=0x01;
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
		if ((abs(Newcurtime-sendserchtime) > 600 * CLOCKS_PER_SEC)) //大于900秒就是15分钟
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

		if (abs(Newcurtime-OneTime)>4 * CLOCKS_PER_SEC)//读取一次数据
		{
			OneTime=Newcurtime;
			SendASDU21(0x01,0x01);  //读取一次数据
			return true;
		}
		if (abs(Newcurtime-TwoTime)>4 * CLOCKS_PER_SEC)//读取二次数据
		{
			TwoTime=Newcurtime;
			SendASDU21(0x02,0x02); //读取二次数据
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
		case 0x32://ASDU50 遥测
			{
			    ASDU0x32(pbuf);
				//EXpainYc(pbuf);
				break;
			}
		case 0x2c://全遥信
			{
				EXpainYx(pbuf);
				break;
			}
		case 0x28://ASDU40 单点全遥信或遥信变位
			{
				//EXpainBwYx(pbuf);
				ASDU0x28(pbuf);
				break;
			}
        case 0x2A://双点全遥信或遥信变位
			{
				ASDU2a(pbuf);
				break;
			}
		case 0x29://单点SOE
			{
				//EXpainSOE(pbuf);
				ASDU0x29(pbuf);
				break;
			}
        case 0x2B:
            {
                ASDU0x2B(pbuf);
            }
		case 0x0a:
			{
                ASDU10(pbuf);//解遥测
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
        case 0x40:  //YK reply
            {
                if(m_bTask)
                {
                    if(pbuf[8]==0x0c)
                    {
                        ShowMessage("YK successful");
                        m_curTask.taskResult.resultCode = 0;
                        m_curTask.isTransfer = 1;
                        Transfer(&m_curTask);
                        memset(&m_curTask, 0, sizeof(m_curTask));
                    }
                    else if(pbuf[8]==0x50)
                    {
                        ShowMessage("YK error cot is 0x50");
                    }
                    break ;
                }
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

ST_BOOLEAN	C103::IsSupportEngine(ST_INT engineType)
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

void	C103::SendASDU21(ST_BYTE Rill,ST_UINT16 GIN)
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
	sendbuf[12]=Rill;  //Rii
	sendbuf[13]=0x01;  //NOG
	sendbuf[14] = GIN % 256;	//GIN
	sendbuf[15] = GIN / 256;
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
    int fun = Rebuf[10];
/*    if(fun != 0x10 || fun != 0x11)
        return ;*/

    char msg[64];
    sprintf(msg,"enter 2A fun: %d",fun);
    ShowMessage(msg);

	for (int i=0;i<yccount;i++)
	{
        int inf = Rebuf[11 + 2 * i];
        fvalue  = Rebuf[12 + 2 * i] == 0x02?1:0;
        if(fun == 0x10){
            this->UpdateValue(getfun16VarId(inf),fvalue);
        }
        else if(fun == 0x11){
            this->UpdateValue(getfun17VarId(inf)+100,fvalue);
        }

	}


}

void	C103::ASDU10(ST_BYTE* Rebuf)
{
    unsigned char fun = Rebuf[10];
    unsigned char inf = Rebuf[11];

    if(fun == 0xFE && inf == 0xF9)  //遥控选择
    {
        if(Rebuf[8]==0x28)
            this->m_curTask.taskResult.resultCode = 0;
        else
            this->m_curTask.taskResult.resultCode = -1;
        m_curTask.isTransfer = 1;
        Transfer(&m_curTask);
        memset(&this->m_curTask,0,sizeof(m_curTask));
    }
    if(fun == 0xFE && inf == 0xFA)  //遥控执行
    {
        if(Rebuf[8]==0x28)
            this->m_curTask.taskResult.resultCode = 0;
        else
            this->m_curTask.taskResult.resultCode = -1;
        m_curTask.isTransfer = 1;
        Transfer(&m_curTask);
        memset(&this->m_curTask,0,sizeof(m_curTask));
    }



	ST_BYTE yccount,funit;
	yccount=Rebuf[13]&0x3f;
	float fvalue=0;
	int fIndex = 14;
	ShowMessage("analyzer!");
	for (int i=0;i<yccount;i++)
	{

        fun = Rebuf[fIndex];
        inf = Rebuf[fIndex+1];
        int type = Rebuf[fIndex+3];
        int datalen = Rebuf[fIndex+4];
        ST_BYTE *vbegin =  &Rebuf[fIndex+6];


        if(type == 0x07){
            char bvalue[4];
            bvalue[0]=vbegin[0];
            bvalue[1]=vbegin[1];
            bvalue[2]=vbegin[2];
            bvalue[3]=vbegin[3];
            fvalue =*(float*)(bvalue);

            //char msg[64];
            //sprintf(msg,"f")
            int vID = 30000;
            if(fun == 0x01)
                vID = inf+10000-1;  //一次数据
            else if(fun == 0x02)
                vID = inf+20000-1;  //二次数据

/*            char msg[128];
            sprintf(msg,"fun :%d,inf :%d , value :%f,vID :%d",fun,inf,fvalue,vID);
            ShowMessage(msg);*/
            this->UpdateValue(vID,fvalue);

        }
        else
        {
            char msg1[64];
            sprintf(msg1,"type is not suitable type: %d",type);

            //ShowMessage(msg1);

        }


        fIndex += datalen + 6 ;
	}
}

void	C103::ASDU1(ST_BYTE* Rebuf)
{
	ST_BYTE count = Rebuf[7] & 0x7F;

	unsigned char fun = Rebuf[10];
    unsigned char inf = Rebuf[11];
/*    if(fun != 0x10 || fun != 0x11)
        return ;*/

     if (!(Rebuf[7] & 0x80)) {
		for (int i = 0; i < count; ++i)
		{
		    ST_BYTE fvalue =Rebuf[12 + 5 * i] == 0x02?1:0; //(Rebuf[12 + 5 * i] & 0x01)==2?1:0;
			if(fun == 0x10)
                this->UpdateValue(getfun16VarId(inf),fvalue);
            else if(fun == 0x11)
                this->UpdateValue(getfun17VarId(inf)+100,fvalue);
            else if(fun == 0x0B){
                this->UpdateValue(getfun16VarId(inf),fvalue);
                ShowMessage("0B");
            }
            if(fun == 0x0D){
                    /*
                this->UpdateValue(getfun17VarId(inf)+100,fvalue);
                ShowMessage("0C");*/
                if(inf == 0x55)
                {
                    ShowMessage("0D55");
                    for(int j = 0;j<40;j++)
                    {
                        this->UpdateValue(j+100,0);
                    }
                }
            }

		}
    }
    else {
        for (int i = 0; i < count; ++i){
            unsigned char fun = Rebuf[10 + 5*i];
            unsigned char inf = Rebuf[11 + 5*i];

            ST_BYTE fvalue =Rebuf[12 + 5 * i] == 0x02?1:0; //(Rebuf[12 + 5 * i] & 0x01)==2?1:0;
            if(fun == 0x10){
                this->UpdateValue(getfun16VarId(inf),fvalue);
                //ShowMessage("10");
            }
            else if(fun == 0x11){
                this->UpdateValue(getfun17VarId(inf)+100,fvalue);
                //ShowMessage("11");
            }
            if(fun == 0x0B){
                this->UpdateValue(getfun16VarId(inf),fvalue);
                ShowMessage("0B");
            }
            if(fun == 0x0D){

                if(inf == 0x55)
                {
                    ShowMessage("0D55");
                    for(int j = 0;j<40;j++)
                    {
                        this->UpdateValue(j+100,0);
                    }
                }

            }

        }
    }
}


void	C103::ASDU2(ST_BYTE* Rebuf)
{

    unsigned char fun = Rebuf[10];
    unsigned char inf = Rebuf[11];
/*    if(fun != 0x10 || fun != 0x11)
        return ;*/

    ST_BYTE bvalue;
    bvalue= Rebuf[12] == 2?1:0;
    int nhour,nminte,nmillsecond;
    nhour=Rebuf[16+4]&0x7f;
    nminte=Rebuf[15+4]&0x1f;
    nmillsecond=Rebuf[13+4]+Rebuf[14+4]*256;
    int addrs;
    if(fun == 0x10){
        addrs = getfun16VarId(inf);
        this->UpdateValue(addrs,bvalue);
        TransferEx (bvalue, addrs, nhour, nminte, nmillsecond);
    }

    else if(fun == 0x11){
        addrs = getfun17VarId(inf)+100;
        this->UpdateValue(addrs,bvalue);
        TransferEx (bvalue, addrs, nhour, nminte, nmillsecond);
    }

    else if(fun == 0xA0){
        ShowMessage("A0");
        addrs = getfun17VarId(inf)+100;
        this->UpdateValue(addrs,bvalue);
        TransferEx (bvalue, addrs, nhour, nminte, nmillsecond);
    }

/*
    this->UpdateValue(addrs,bvalue);
    TransferEx (bvalue, addrs, nhour, nminte, nmillsecond);*/

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
/*    ST_BYTE sendcode,Code,FCV;
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
	data[ 6] = 0x40;    //type
	data[ 7] = 0x81;    //vsq
	data[ 8] = 0x12;    //cot remote operation
	data[ 9] = data[5];
	data[10] = 0x01;    //FUN
	data[11] = 0x30;    //INF 48:YK1
	data[12] = (task.taskValue == 2 ? 0x82: 0x81);
    data[13] = 0x35;    // RII
    ST_BYTE cssum=0;
	for (int i=4;i<14;i++)
	{
		cssum=cssum+data[i];
	}
	data[14] = cssum;
	data[16] = 0x16;
	this->Send(data, 16);*/
	//68 11 11 68 53 01 0A 81 28 01 FE F9 02 01 06 03 01 09 01 01 02 19 16
	ST_BYTE sendcode,Code,FCV;
	FCV=1;
	Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE sendbuf[20] = {0};
	sendbuf[0]=0x68;
	sendbuf[1]=0x11;
	sendbuf[2]=0x11;
	sendbuf[3]=0x68;
	sendbuf[4]=sendcode;
	sendbuf[5]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6]=0x0A; //adsu
	sendbuf[7]=0x81; //vsq
	sendbuf[8]=0x28;
	sendbuf[9]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10]=0xfe; //fun
	sendbuf[11]=0xf9;  //带确认的写条目 实现遥控选择
	sendbuf[12]=0x02;  //Rii
	sendbuf[13]=0x01;  //NOG 以下n组描述
	sendbuf[14]=0x06;
	sendbuf[15]=task.taskValue == 1 ? 0x03: 0x04;//0x03;  //GIN
	sendbuf[16]=0x01;  //KOD 实际值
	sendbuf[17]=0x09;   //GDD
	sendbuf[18]=0x01;       //数据宽度
	sendbuf[19]=0x01;       //后续状态
	sendbuf[20]=0x02;//task.taskValue == 1 ? 0x02: 0x01;
	ST_BYTE cssum=0;
	for (int i=4;i<21;i++)
	{
		cssum=cssum+sendbuf[i];
	}
	sendbuf[21]=cssum;
	sendbuf[22]=0x16;
	 this->Send(sendbuf,23);
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
{/*
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
	data[ 6] = 0x40;    //type
	data[ 7] = 0x81;    //vsq
	data[ 8] = 0x12;    //cot remote operation
	data[ 9] = data[5];
	data[10] = 0x01;    //FUN
	data[11] = 0x30;    //INF 48:YK1

	data[12] = (task.taskValue == 2 ? 0x02: 0x01);
    data[13] = 0x35;    // RII
    ST_BYTE cssum=0;
	for (int i=4;i<14;i++)
	{
		cssum=cssum+data[i];
	}
	data[14] = cssum;
	data[16] = 0x16;
	this->Send(data, 16);*/
	//68 11 11 68 73 01 0A 81 28 01 FE FA 03 01 06 03 01 09 01 01 02 3B 16
	ST_BYTE sendcode,Code,FCV;
	FCV=1;
	Code=3;
	sendcode=0x40;//PRM
	sendcode=sendcode|(FCB<<5);
	sendcode=sendcode|(FCV<<4);
	sendcode=sendcode|Code;
	ST_BYTE sendbuf[20] = {0};
	sendbuf[0]=0x68;
	sendbuf[1]=0x11;
	sendbuf[2]=0x11;
	sendbuf[3]=0x68;
	sendbuf[4]=sendcode;
	sendbuf[5]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6]=0x0A; //adsu
	sendbuf[7]=0x81; //vsq
	sendbuf[8]=0x28;
	sendbuf[9]=this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10]=0xfe; //fun
	sendbuf[11]=0xFA;  //带确认的写条目 实现遥控选择
	sendbuf[12]=0x03;  //Rii
	sendbuf[13]=0x01;  //NOG 以下n组描述
	sendbuf[14]=0x06;
	sendbuf[15]=task.taskValue == 1 ? 0x03: 0x04;//0x03;  //GIN
	sendbuf[16]=0x01;  //KOD 实际值
	sendbuf[17]=0x09;   //GDD
	sendbuf[18]=0x01;       //数据宽度
	sendbuf[19]=0x01;       //后续状态
	sendbuf[20]=0x02;//task.taskValue == 1 ? 0x02: 0x01;
	ST_BYTE cssum=0;
	for (int i=4;i<21;i++)
	{
		cssum=cssum+sendbuf[i];
	}
	sendbuf[21]=cssum;
	sendbuf[22]=0x16;
	 this->Send(sendbuf,23);
}

//遥测
void    C103::ASDU0x32(ST_BYTE* Rebuf)
{
 //   this->ShowMessage("Enter asdu0x32");
	ST_BYTE yccount = Rebuf[7] & 0x7f;
	if (Rebuf[7] & 0x80) { // sq
		for (int i = 0; i < yccount; ++i)
		{
			unsigned char fun = Rebuf[10 + 4 * i];
			unsigned char inf = Rebuf[11 + 4 * i];

			float value = 0;
			unsigned short bits = Rebuf[12 + 4 * i] + Rebuf[13 + 4 * i] * 256;

			if (bits & 0x8000)
				value = -1 * (((~((bits >> 3) & 0x1FFF)) & 0x1FFF) + 1);
			else
				value = ((bits >> 3) & 0x1FFF);
            float cvalue = calCoefficient(inf,value);
           // char dataValue[64];
           // sprintf(dataValue,"inf:%d ,data value:%f ,id:%d",inf,value,(inf - 92));
           // this->ShowMessage(dataValue);
			this->UpdateValue(10000+inf-92, cvalue);
		}
	}
	else {
		float fvalue = 0.0;
		unsigned char fun = Rebuf[10];
		unsigned char inf = Rebuf[11]-92;
        this->ShowMessage("Enter asdu0x32  01");
		for (int i = 0; i < yccount; ++i)
		{
			float value = 0;
			unsigned short bits = Rebuf[13 + 2 * i] * 256 + Rebuf[12 + 2 * i];
			if (bits & 0x8000)
				value = -1 * (((~((bits >> 3) & 0x1FFF)) & 0x1FFF) + 1);
			else
				value = ((bits >> 3) & 0x1FFF);
			this->UpdateValue(10000+inf , value);
		}
	}
}

void C103::ASDU0x29(ST_BYTE * Rebuf)
{
    ST_BYTE fun,inf;
    if(Rebuf[10]==0xC0)
    {
        fun=Rebuf[10];
        inf=Rebuf[11]-129;
        ST_BYTE bvalue;
        bvalue=Rebuf[12];
        int nhour,nminte,nmillsecond;
        nhour=Rebuf[16];
        nminte=Rebuf[15];
        nmillsecond=Rebuf[13]+Rebuf[14]*256;

        TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
    }

}

//68  0e  0e  68  08   0f  2B  81  01  0f  01  96  02  18  71  08  0e  01  0c 16
void C103::ASDU0x2B(ST_BYTE * Rebuf)
{
    ST_BYTE fun,inf;
	fun=Rebuf[10];
	inf=Rebuf[11]-149;
	ST_BYTE bvalue;
	bvalue=(Rebuf[12]&0x03)==0x02?1:0;
	int nhour,nminte,nmillsecond;
	nhour=Rebuf[16];
	nminte=Rebuf[15];
	nmillsecond=Rebuf[13]+Rebuf[14]*256;

	TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
}

void C103::ASDU0x28(ST_BYTE * Rebuf)
{
	ST_BYTE count = Rebuf[7] & 0x7F;
	if (!(Rebuf[7] & 0x80)) {

        unsigned char fun = Rebuf[10];
        unsigned char inf = Rebuf[11];
		for (int i = 0; i < count; ++i)
		{

			this->UpdateValue(inf - 149, Rebuf[12 + i] & 0x01);
		}
	}
	else {
		for (int i = 0; i < count; ++i)
		{
			unsigned char fun = Rebuf[10 + 3*i];
			unsigned char inf = Rebuf[11 + 3*i];

			ST_BYTE fvalue = Rebuf[12 + 3 * i] & 0x01;
			this->UpdateValue(inf - 149, fvalue);
		}
	}
}



float   C103::calCoefficient(int inf,float fvalue)
{
    float cfvalue = 0;
    switch(inf)
    {
        case 92 :
        case 93 :
        case 94 :
        case 96 :
        case 97 :
        case 98 :
        case 100:
        case 101:
        case 102:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
            cfvalue = fvalue/17.066667;
            break;
        case 95 :
        case 99 :
        case 103:
            cfvalue = fvalue/7.757576;
            break;
        case 113:
        case 114:
        case 115:
            cfvalue = fvalue/102.4;
            break;
        default:
            cfvalue = fvalue;
            break;
    }
    return cfvalue;
}

int     C103::getfun16VarId(int inf)
{
    int id = -1;
    switch(inf){
    case 1: id = 0;break;
    case 2: id = 1;break;
    case 3: id = 2;break;
    case 4: id = 3;break;
    case 5: id = 4;break;
    case 6: id = 5;break;
    case 10: id = 6;break;
    case 11: id = 7;break;
    case 12: id = 8;break;
    case 13: id = 9;break;
    case 14: id = 10;break;
    case 15: id = 11;break;
    case 16: id = 12;break;
    case 17: id = 13;break;
    case 18: id = 14;break;
    case 19: id = 15;break;
    default:
        break;
    }
    return id;
}
int     C103::getfun17VarId(int inf)
{
    int id = -1;
    switch(inf){
    case 30: id = 0;break;
    case 32: id = 1;break;
    case 34: id = 2;break;
    case 40: id = 3;break;
    case 42: id = 4;break;
    case 44: id = 5;break;
    case 50: id = 6;break;
    case 52: id = 7;break;
    case 56: id = 8;break;
    case 58: id = 9;break;
    case 68: id = 10;break;
    case 70: id = 11;break;
    case 72: id = 12;break;
    case 76: id = 13;break;
    case 92: id = 14;break;
    case 96: id = 15;break;
    case 102: id = 16;break;
    case 110: id = 17;break;
    case 118: id = 18;break;
    case 122: id = 19;break;
    case 126: id = 20;break;
    case 132: id = 21;break;
    case 136: id = 22;break;
    case 142: id = 23;break;
    case 151: id = 24;break;
    case 156: id = 25;break;
    case 158: id = 26;break;
    case 160: id = 27;break;
    case 161: id = 28;break;
    case 202: id = 29;break;
    case 204: id = 30;break;
    case 208: id = 31;break;
    case 221: id = 32;break;
    case 222: id = 33;break;
    case 223: id = 34;break;
    case 224: id = 35;break;
    case 225: id = 36;break;
    case 226: id = 37;break;
    case 227: id = 38;break;
    case 228: id = 39;break;
    default:
        break;
    }
    return id;
}
