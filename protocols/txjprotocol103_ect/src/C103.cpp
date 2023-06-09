#include "C103.h"
#include "Channel.h"
#include "datetime.h"
#include "time.h"

volatile static C103::YKStep step = C103::undefined;
volatile static bool bsOK = false;
volatile bool resLinkS = false;
volatile ST_BYTE muxian = 0x00;
volatile ST_BYTE jinxian = 0x00;
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
    time(&Newcurtime);//=clock();
    time(&oldcurtime);//=clock();
    time(&sendserchtime);//=clock();
    time(&m_ivRead);// = clock();
    CLtime=sendserchtime;
    BreakCallState=0;
    sendflag=0;
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
                m_pLogger->LogDebug("fixed frame:len = %d",len);
                return;
            }
            else if (pbuf[0]==0x68)//收到的是可变帧
            {
                len=pbuf[1]+6;
                this->GetCurPort()->ReadBytes(pbuf,len,3000);
                readed=pbuf[1]+6;
                m_pLogger->LogDebug("long frame:len = %d",len);
                return;
            }
            else
            {
                this->GetCurPort()->Clear();//清空乱码
                return;
            }
        }
        else
        {
            //  clock_t nowtime = clock();
            time(&t1);
//            if(((nowtime - m_ivRead) )>15*CLOCKS_PER_SEC)
            if((t1-t2)>5)
            {
                this->UpdateValue(41,1);
                SendState = 0x01;
                resLinkS = true;
                ShowMessage("timeout receive!");
            }

        }
    }
}

ST_BOOLEAN	C103::OnSend()
{
    time(&Newcurtime);//=clock();
    ST_BYTE FCV,Code;
    ST_BYTE sendcode=0;
    if (this->HasTask() && this->GetTask(&m_curTask))
    {
        /*
        if (m_curTask.taskCmdCode==0x01)//对时
        {
        	SendASDU6();
        	m_curTask.taskResult.resultCode = 0;
        	Transfer (&m_curTask);
        	memset(&m_curTask,0,sizeof(m_curTask));
        	return false;
        }*/
        if (!strcmp(m_curTask.taskCmd, "devicecontrol"))
        {
            switch (m_curTask.taskCmdCode)
            {
            //遥控选择
            case 0:
            {
                ShowMessage("receive YK select");
                m_curTask.taskResult.resultCode = 0;
                Transfer (&m_curTask);
                memset(&m_curTask,0,sizeof(m_curTask));
            }
            break;
            //遥控执行
            case 1:
            {
                ShowMessage("receive YK execute");
                SendState = 0x00;
                step = SEND_PRE_YK;
            }
            break;
            //遥控取消
            case 2:
            {
            } break;
            default:
                break;
            }
        }
    }

    if (SendState==0x00) //复位
    {
        FCV=0;
        Code=0;
        sendcode=0x40;//PRM
        FCB =0;
        sendcode=sendcode|(FCB<<5);
        sendcode=sendcode|(FCV<<4);
        sendcode=sendcode|Code;
        SendSetframe(sendcode);
    }
    else if (SendState==0x01)//请求链路状态
    {
        FCB = 0;
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
        if (((Newcurtime-sendserchtime) > 10*60)) //10分钟一次总召  //大于1800秒就是30分钟
        {
            time(&sendserchtime);//=clock();
            SendAllSearch();
            return true;
        }
        if (BreakCallState)
        {
            SendAllSearch();
            BreakCallState=0;
            return true;
        }
        if ((Newcurtime-oldcurtime)>50) //大于3m
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
    else if(SendState==0x05) //遥控选择
    {
        FCV = 1;
        Code = 3;      //功能码3
        sendcode = 0x40;//PRM
        sendcode = sendcode | (FCB << 5);
        sendcode = sendcode | (FCV << 4);
        sendcode = sendcode | Code;
        bsOK = m_curTask.taskValue;
        SendPreYK(sendcode,0x20,bsOK);
        step = SEND_PRE_YK_END;
    }
    else if(SendState==0x06) //遥控执行
    {
        FCV = 1;
        Code = 3;      //功能码3
        sendcode = 0x40;//PRM
        sendcode = sendcode | (FCB << 5);
        sendcode = sendcode | (FCV << 4);
        sendcode = sendcode | Code;
        SendYK(sendcode, 0x20, bsOK);
        step = undefined;
    }
    return true;
}
ST_BOOLEAN	C103::OnProcess(ST_BYTE* pbuf,int len)
{
    time(&m_ivRead);// = clock();
    time(&t2);
    time(&oldcurtime);//=clock();
    ST_BYTE DFC,ACD,Recode;
    this->UpdateValue(41,0);
    FCB = !FCB;
    int maddr = this->GetDevice()->GetDeviceInfo()->Address;
    if(maddr==1)
    {
        this->UpdateValue(7,1);
    }
    else if(maddr==2)
    {
        this->UpdateValue(7,0);
    }
    if (pbuf[0]==0x10)//固定帧
    {
        Recode=pbuf[1]&0x0f;
        DFC=(pbuf[1]&0x10)>>4;
        ACD=(pbuf[1]&0x20)>>5;
        if (Recode == 0x00)//确认
        {
            switch (step)
            {
            case SEND_PRE_YK:
            {
                FCB = 0;   //重启链路重新置0
                SendState = 2;  //召唤一级数据

                return true;
            }
            break;
            case SEND_PRE_YK_END:
            {
                FCB = 1;
                SendState = 2;  //召唤一级数据

                return true;
            }
            break;
            case SEND_YK:
            {
                m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                Memset(&m_curTask, 0, sizeof(m_curTask));
            }
            break;
            default:
                break;
            }
        }
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
            if (step == SEND_PRE_YK)
            {
                SendState = 2;

            }
        }
        else if (Recode==0x01)//链路忙
        {
        }
        else if (Recode==0x09)//从站没有所召唤的数据
        {
            SendState=3; //召唤二级数据
            if (step != undefined)
            {
                SendState = 5;
            }
//			ShowMessage("Enter recode 0x09");
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
        case 0x31://循环遥测
        {
            EXpainYc(pbuf);
            break;
        }
        //遥信状态报文不仅仅在总召唤时上传，在有遥信变位时也传送
        case 0x28://全遥信     old://变位遥信
        {
            EXpainYx(pbuf);
        }
        break;
        case 0x2B:
        {
            m_pLogger->LogDebug("before ExplainSOE");
            EXpainSOE(pbuf);
            m_pLogger->LogDebug("ExplainSOE executed");

        }
        break;
        case 0x01:
        {
            ASDU1(pbuf);//告警
            if (step == SEND_PRE_YK_END)
            {
                SendState = 0x06;
                return true;
            }
            break;
        }
        case 0x40:
        {
            m_curTask.taskResult.resultCode = 0;
            Transfer (&m_curTask);
            memset(&m_curTask,0,sizeof(m_curTask));
            break;
        }
        case 0x05:
        {
            SendState = 2;

            /*
            if (step == SEND_PRE_YK)
            {
            SendState = 2;
            return true;
            }*/
            return true;
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
    int maddr = this->GetDevice()->GetDeviceInfo()->Address;
    struct tm t_tm;
    DateTime::localtime (time(0), t_tm); //获取时间
    ST_BYTE yccount,funit;
    yccount=Rebuf[7]&0x7f;
    float fvalue=0;
    for (int i=0; i<yccount; i++)
    {
        float coefficient = this->GetDevice()->GetDeviceInfo()->DataAreas[1].items[i].coeficient;
        fvalue = ((Rebuf[13 + 2 * i] * 256 + Rebuf[12 + 2 * i]))*coefficient;
        this->UpdateValue(10000+i,fvalue);
        if(maddr<3)  //设备1和2 都是电压型继保
        {
            if(i==7)
            {
                //母线有压，则将信号点置1
                if(fvalue>0)
                {
                    this->UpdateValue(37,1);
                    if(muxian == 0x00)
                    {
                        muxian == 0x01;
                        TransferEx (1, 37, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }
                }
                else if(fvalue==0) //母线无压，则将信号点置0
                {
                    this->UpdateValue(37,0);
                    if(muxian == 0x01)
                    {
                        muxian = 0x00;
                        TransferEx (0, 37, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }

                }
            }
            if(i==5)
            {
                if(fvalue>0)
                {
                    this->UpdateValue(42,1);
                    if(jinxian == 0x00)
                    {
                        jinxian == 0x01;
                        TransferEx (1, 42, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }
                }
                else if(fvalue==0)
                {
                    this->UpdateValue(42,0);
                    if(jinxian == 0x01)
                    {
                        jinxian == 0x00;
                        TransferEx (0, 42, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }

                }
            }
        }
        else   //设备3,4,5 都是电流型继保
        {
            if(i==14)
            {
                if(fvalue>0)
                {
                    if(muxian == 0x00)
                    {
                        muxian = 0x01;
                        this->UpdateValue(36,1);
                        TransferEx (1, 36, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }

                }
                else if(fvalue==0)
                {
                    if(muxian == 0x01)
                    {
                        muxian = 0x00;
                        this->UpdateValue(36,0);
                        TransferEx (0, 36, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec*1000);
                    }

                }
            }
        }
    }
}
void    C103::EXpainYx(ST_BYTE* Rebuf)
{
    ST_BYTE yxcount,funit;
    ST_BYTE bvalue;
    yxcount=Rebuf[7]&0x7f;
    int maddr = this->GetDevice()->GetDeviceInfo()->Address;
    for (int k=0; k<yxcount; k++)
    {
        if(maddr<3)
        {
            if(k==37)
            {

            }
            else if(k==42)
            {

            }
            else
            {
                bvalue = ((Rebuf[12 + k])==0x02?0x01:0x00);
                this->UpdateValue(k,bvalue);
            }
        }
        else
        {
            if(k==36)
            {

            }
            else
            {
                bvalue = ((Rebuf[12 + k])==0x02?0x01:0x00);
                this->UpdateValue(k,bvalue);
            }
        }
    }

}
void    C103::EXpainBwYx(ST_BYTE* Rebuf)
{

}

//68 18 18 68 28 01 2B 81 01 01 FA 39 02 00 00 00 00 00 E6 AB 30 1B 08 1F 00 00 00 00 0F 16
void    C103::EXpainSOE(ST_BYTE* Rebuf)
{
    ST_BYTE fun,inf;
    fun=Rebuf[10];
    inf=Rebuf[11]-0x10;
    ST_BYTE bvalue;
    bvalue=Rebuf[12]==0x02?0x01:0x00;
    int nhour,nminte,nmillsecond;
    nhour=Rebuf[21];
    nminte=Rebuf[20];
    nmillsecond=Rebuf[18]+Rebuf[19]*256;
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
    if(inf == 37)
    {
        return ;
    }
    this->UpdateValue(inf,bvalue);
    TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
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
    for (int i=4; i<13; i++)
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
    for (int i=4; i<17; i++)
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

    for (int i=0; i<yccount; i++)
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
    for (int i=0; i<yccount; i++)
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
    if (Rebuf[11]>=0x10 ) //SOE
    {
        inf=abs(Rebuf[11]-0x10);
        ST_BYTE bvalue;
        bvalue = (Rebuf[12] == 2 ? 0x01 : 0x00);
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
    for (int i=4; i<19; i++)
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

void  C103::SendPreYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn)
{
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x68;
    sendbuf[1] = 0x0A;
    sendbuf[2] = 0x0A;
    sendbuf[3] = 0x68;
    sendbuf[4] = fc;
    sendbuf[5] = this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = 0x40;
    sendbuf[7] = 0x01;
    sendbuf[8] = 0x14;     //0x0C;
    sendbuf[9] = this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10] = 0xB2;
    sendbuf[11] = 0x20;
    sendbuf[12] = bYkOn ? 0x82 : 0x81;
    sendbuf[13] = 0x00;
    ST_BYTE cssum = 0;
    for (int i = 4; i < 14; i++)
    {
        cssum = cssum + sendbuf[i];
    }
    sendbuf[14] = cssum;
    sendbuf[15] = 0x16;
    this->Send(sendbuf, 16);
    this->ShowMessage("Remote control select");
}

void  C103::SendYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn)
{
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x68;
    sendbuf[1] = 0x0A;
    sendbuf[2] = 0x0A;
    sendbuf[3] = 0x68;
    sendbuf[4] = fc;
    sendbuf[5] = this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = 0x40;
    sendbuf[7] = 0x01;
    sendbuf[8] = 0x14;
    sendbuf[9] = this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10] = 0xB2;     //电流或电压型
    sendbuf[11] = 0x20;     //wAddr;
    sendbuf[12] = bYkOn ? 0x02 : 0x01;
    sendbuf[13] = 0x00;
    ST_BYTE cssum = 0;
    for (int i = 4; i < 14; i++)
    {
        cssum = cssum + sendbuf[i];
    }
    sendbuf[14] = cssum;
    sendbuf[15] = 0x16;
    this->Send(sendbuf, 16);
    this->ShowMessage("Remote control execute");
}

void  C103::SendEndYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn)
{

}
