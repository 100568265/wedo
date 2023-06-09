#include "mn9600.h"
#include "Device.h"
#include "Channel.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

mn9600 * CreateInstace()
{
    return new mn9600();
}

mn9600::mn9600()
{
    m_readIndex = 0;
    //ctor
}

mn9600::~mn9600()
{
    //dtor
}

void mn9600::Init()
{}

void mn9600::Uninit()
{}

ST_BOOLEAN  mn9600::IsSupportEngine (ST_INT IsSupportEngine)//半双工
{
    return true;
}

void mn9600::OnRead(ST_BYTE* pbuf,int& readed)
{
    readed = 0;
    if(! this->GetCurPort())//如果没有通道
        return;

    int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;//延迟时间等于通讯间隔
    if (lineInterval < 3000)
        lineInterval = 3000;

    int len = this->GetCurPort()->PickBytes(pbuf, 4, lineInterval);

    if(len >= 4)//最短长度
    {
        int star = 0;
        for(; star < len; ++star)
        {
            if(pbuf[star] == ((uint16_t)this->GetDevice()->GetDeviceInfo()->Address)>>5)//寻找地址,ID=(ADDR<<3)|5
                break;
        }
        if(star > 0)
        {
            //star大于0，说明有乱码， 把之前的乱码丢掉
            this->GetCurPort()->ReadBytes(pbuf, star);
        }
        if(len == star)
        {
            //全是乱码，清除乱码
            ShowMessage ("Garbled code, clear buffer.");
            this->GetCurPort()->Clear();
            return;
        }

        len = this->GetCurPort()->PickBytes(pbuf, 4, lineInterval);

        uint8_t datalength = pbuf[4];//上传数据总字节数

        len = (datalength%8?((datalength/8+1)*3):(datalength/8*3))+datalength;//报文长度=每帧标志符+上传数据字节数

        this->GetCurPort()->ReadBytes(pbuf, len);

        readed = len;

    }
    else
    {
        ShowMessage ("Insufficient data length.");//数据长度不足
        this->GetCurPort()->Clear();
    }
}

ST_BOOLEAN	mn9600::OnSend()
{
    if (this->GetCurPort())
        this->GetCurPort()->Clear();

    m_bTask = false;
    if(this->HasTask() && this->GetTask(&m_curTask))
    {
        if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
            if(m_curTask.taskCmdCode == 0)
                SendYK(m_curTask.taskValue);//发送遥控
            else if(m_curTask.taskCmdCode == 1)
                ConfirmYK(m_curTask.taskValue);//确认遥控
            else if(m_curTask.taskCmdCode == 2)
                CancelYK(m_curTask.taskValue);//取消遥控
            else
            {
                m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                memset(&m_curTask,0,sizeof(m_curTask));
                return false;
            }
            m_bTask = true;
        }
        return true;
    }


    if(m_readIndex<20)
    {
        ReadData1();

    }
    else if(m_readIndex<22&&m_readIndex>=20)
    {
        ReadData2();

    }
    else if(m_readIndex<37&&m_readIndex>=22)
    {
        ReadData3();

    }
    m_readIndex = (++m_readIndex % 37);
//	return true;
//                                else if(m_readIndex>36)
//                                {
//                                                m_readIndex = 0;
//
//                                }
//                                return true;
//	}
    return true;
}

bool mn9600::OnProcess(ST_BYTE* pbuf, int len)
{
    if(pbuf[3] == 0x59)//遥信
    {
        EXpainYx(pbuf);
    }
    else  if(pbuf[3] == 0x56)//遥测
    {
        EXpainYc(pbuf);
    }
    else  if(pbuf[3] == 0x58)//电度
    {
        EXpainEq(pbuf);
    }
    else  if(pbuf[3] == 0x40)//SOE
    {
        EXpainSOE(pbuf);
    }
    return true;
}



void mn9600::SendYK(ST_BOOLEAN bIsOn)//申请遥控
{
    ST_BYTE sendbuf[32] = {0};
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;//标志符
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 || 5;//标志符
    sendbuf[2] = 0x08;//DLC
    sendbuf[3] = 0x5A;//功能码
    sendbuf[4] = 0x08;
    sendbuf[5] = 0x01;//Jx序号
    sendbuf[6] = (bIsOn ? 0xCC: 0x33);//DATA命令，分闸为0合闸为1
    sendbuf[7] = 0x00;
    sendbuf[8] = 0x00;
    sendbuf[9] = 0x00;
    sendbuf[10] = 0x00;
    this->Send(sendbuf,11);
}

void mn9600::ConfirmYK(ST_BOOLEAN bIsOn)//确认遥控
{
    ST_BYTE sendbuf[32] = {0};
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;//标志符
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 || 5;//标志符
    sendbuf[2] = 0x08;//DLC
    sendbuf[3] = 0x5B;//功能码
    sendbuf[4] = 0x08;
    sendbuf[5] = 0x01;//Jx序号
    sendbuf[6] = (bIsOn ? 0xCC: 0x33);//DATA命令，分闸为0合闸为1
    sendbuf[7] = 0x00;
    sendbuf[8] = 0x00;
    sendbuf[9] = 0x00;
    sendbuf[10] = 0x00;
    this->Send(sendbuf,11);
}

void mn9600::CancelYK(ST_BOOLEAN bIsOn)//取消遥控
{
    ST_BYTE sendbuf[32] = {0};
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;//标志符
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 || 5;//标志符
    sendbuf[2] = 0x08;//DLC
    sendbuf[3] = 0x5C;//功能码
    sendbuf[4] = 0x01;//Jx序号
    sendbuf[5] = (bIsOn ? 0xCC: 0x33);//DATA命令，分闸为0合闸为1
    this->Send(sendbuf,11);
}


void mn9600::ReadData1()//发送遥测
{
    ST_BYTE sendbuf[4];
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 | 5;
    sendbuf[2] = 0x01;
    sendbuf[3] = 0x56;
    this->Send(sendbuf,4);
}

void mn9600::ReadData2()//发送电量
{
    ST_BYTE sendbuf[4];
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 | 5;
    sendbuf[2] = 0x02;
    sendbuf[3] = 0x58;
    this->Send(sendbuf,4);
}

void mn9600::ReadData3()//发送遥信
{
    ST_BYTE sendbuf[4];
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 | 5;
    sendbuf[2] = 0x01;
    sendbuf[3] = 0x59;
    this->Send(sendbuf,4);
}

void mn9600::ReadData4()//读SOE记录
{
    ST_BYTE sendbuf[4];
    sendbuf[0] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) >> 5;
    sendbuf[1] = ((ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address) << 3 | 5;
    sendbuf[2] = 0x02;
    sendbuf[3] = 0x40;
    this->Send(sendbuf,4);
}

void    mn9600::EXpainYx(ST_BYTE* pbuf)
{
    ST_BYTE value =0x00;
    for(int i = 0; i<8; i++) //开关K1到K8的状态
    {
        value = (pbuf[5]>>i)&0x01;
        this->UpdateValue(22+i, (ST_BYTE)value);
    }
    for(int k = 0; k<4; k++) //开关K9到K12的状态
    {
        value = (pbuf[6]>>k)&0x01;
        this->UpdateValue(30+k, (ST_BYTE)value);
    }
    if(pbuf[2] == 5)
    {
        for(int j = 2; j<5; j++) //D3.2,D3.3,D3.4状态
        {
            value = (pbuf[7]>>j)&0x01;
            this->UpdateValue(32+j, (ST_BYTE)value);
        }
    }

}

void    mn9600::EXpainYc(ST_BYTE* pbuf)//低前高后
{
    float fvalue [32] = {0};
    fvalue[0] = (float)(pbuf[5] | pbuf[6]<<8);
    fvalue[1] = (float)(pbuf[7] | pbuf[8]<<8);
    fvalue[2] = (float)(pbuf[9] | pbuf[10]<<8);
    fvalue[3] = (float)(pbuf[14] | pbuf[15]<<8);
    fvalue[4] = (float)(pbuf[16] | pbuf[17]<<8);
    fvalue[5] = (float)(pbuf[18] | pbuf[19]<<8);
    fvalue[6] = (float)(pbuf[20] | pbuf[21]<<8);
    fvalue[7] = (float)(pbuf[25] | pbuf[26]<<8);
    fvalue[8] = (float)(pbuf[27] | pbuf[28]<<8);
    fvalue[9] = (float)(pbuf[29] | pbuf[30]<<8);
    fvalue[10] = (float)(pbuf[31] | pbuf[32]<<8);
    fvalue[11] = (float)(pbuf[36] | pbuf[37]<<8);
    fvalue[12] = (float)(pbuf[38] | pbuf[39]<<8);
    fvalue[13] = (float)(pbuf[40] | pbuf[41]<<8);
    fvalue[14] = (float)(pbuf[42] | pbuf[43]<<8);
    fvalue[15] = (float)(pbuf[47] | pbuf[48]<<8);
    fvalue[16] = (float)(pbuf[49] | pbuf[50]<<8);
    fvalue[17] = (float)(pbuf[51] | pbuf[52]<<8);
    fvalue[18] = (float)(pbuf[53] | pbuf[54]<<8);
    fvalue[19] = (float)(pbuf[58] | pbuf[59]<<8);

    for(int i = 0; i<20; i++)
    {
        this->UpdateValue(i,float(fvalue[i]));
    }


}

void    mn9600::EXpainEq(ST_BYTE* pbuf)
{
    float fvalue1 = (float)(pbuf[5] | pbuf[6]<<8 | pbuf[7]<<16)*0.01;
    float fvalue2 = (float)(pbuf[8] | pbuf[9]<<8 | pbuf[10]<<16)*0.01;
    this->UpdateValue(20,float(fvalue1));
    this->UpdateValue(21,float(fvalue2));
}

void    mn9600::EXpainSOE(ST_BYTE* pbuf)
{
    ST_BYTE typ;
    typ = pbuf[6];
    float fvalue[32];
    fvalue[0] = (float)((pbuf[18] + pbuf[19])<<8)*0.01;//A相保护电流
    fvalue[1] = (float)((pbuf[20] + pbuf[21])<<8)*0.01;//B相保护电流
    fvalue[2] = float((pbuf[25] + pbuf[26])<<8)*0.01;//C相保护电流
    fvalue[3] = float((pbuf[27] + pbuf[28])<<8)*0.1;//AB线电压
    fvalue[4] = float((pbuf[29] + pbuf[30])<<8)*0.1;//BC线电压
    fvalue[5] = float((pbuf[31] + pbuf[32])<<8)*0.1;//CA线电压
    fvalue[6] = float((pbuf[36] + pbuf[37])<<8)*0.1;//零序电压
    fvalue[7] = float((pbuf[38] + pbuf[39])<<8)*0.01;//零序电流
    fvalue[8] = float((pbuf[40] + pbuf[41])<<8)*0.01;//负序电流
    fvalue[9] = float((pbuf[42] + pbuf[43])<<8)*0.01;//频率
    int nyear,nmonth,nday,nhour,nminute,nsecond,nmillsecond;
    nyear = pbuf[7]+2000;
    nmonth = pbuf[8];
    nday = pbuf[9];
    nhour = pbuf[10];
    nminute = pbuf[14];
    nsecond = pbuf[15];
    nmillsecond = pbuf[16]+pbuf[17]*256;
    //TransferEx (fvalue[i], nyear, nmonth, nday, nhour, nminute, nsecond, nmillsecond);
    for(int i = 0; i<10; i++)
    {
        TransferEx (fvalue, nyear, nmonth, nday, nhour, nminute, nsecond, nmillsecond);
    }

}

void    mn9600::TransferEx (float value[ ], ST_BYTE year, ST_BYTE month, ST_BYTE day, ST_BYTE hour, ST_BYTE minute, ST_BYTE second, ST_UINT16 msec)
{
    ProtocolTask task;
    Strcpy(task.taskCmd,"SOE");
    task.isTransfer     = true;
    task.transChannelId = -1;
    task.channelId      = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
    task.deviceId       = this->GetDevice()->GetDeviceInfo()->DeviceId;
    task.taskCmdCode    = 0;
    task.taskParamLen   = 14;
    for(int i = 0; i<10; i++)
    {
        task.taskValue      = value[i];
    }
    task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
    task.ignoreBack     = 1;
    task.taskTime       = 1000;
    task.taskParam[0]   = year;
    task.taskParam[1]   = month;
    task.taskParam[2]   =  day;
    task.taskParam[3]   =  hour;
    task.taskParam[4]   =  minute;
    task.taskParam[5]   =  second;
    task.taskParam[6]   =  minute;//秒
    task.taskParam[7]   = msec % 256;//毫秒低字节
    task.taskParam[8]   = msec  / 256;//毫秒高字节
    task.taskParam[9]   = value[0];
    task.taskParam[10]   = value[1];
    task.taskParam[11]   = value[2];
    task.taskParam[12]   = value[3];
    task.taskParam[13]   = value[4];
    task.taskParam[14]   = value[5];
    task.taskParam[15]   = value[6];
    task.taskParam[16]   = value[7];
    task.taskParam[17]   = value[8];
    task.taskParam[18]   = value[9];
    task.taskParam[19]  = task.taskAddr1 % 256;
    task.taskParam[20]  = task.taskAddr1 / 256;
    Transfer(&task);
}





