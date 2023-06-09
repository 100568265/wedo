#include "CAllGo.h"
#include <stdio.h>
static const ST_CHAR* const STR_TYPE[] =
{
    "智能窗帘控制",
    "温湿度",
    "红外遥控",
    "水浸",
    "智能开关",
    "烟感",
    "XSYM211B",
    "XSYP211B",
    "XSYT212QB",
    "XSYT221B",
    "XSYT231B",
    "XSYT241ZB",
    "XSYT241ZC",
    "XSYA212C"
};

enum SY_Model
{
    AG_ZNCLKZ   = 0x00,
    AG_WSD      = 0x01,
    AG_HWYK     = 0x02,
    AG_SJ       = 0x03,
    AG_ZNKG     = 0x04,
    AG_YG       = 0x05,
    XLast
};
CAllGo::CAllGo()
{
    //ctor
    m_isInit = false;
    m_devSign = -1;
    m_rIndex = 0;

    devstauts = 0;

    time(&Newcurtime);
    time(&oldcurtime);
}

CAllGo::~CAllGo()
{
    //dtor
}

void	CAllGo::Init()
{
    if (!this->GetDevice())
        return;
    string name   = this->GetDevice()->GetDeviceInfo()->Deviceserialtype;
    for (ST_UINT i = 0; i < XLast; ++i)
    {
        if (name == STR_TYPE[i])
            m_devSign = i;
    }

    if(devstauts != 1){
        t_Protocols = m_pChannel->GetCEngine()->m_pProtocols;
        if(t_Protocols.GetCount()<1)
        {
            return ;
        }
        for(int j = 0; j < t_Protocols.GetCount(); j++)
        {
            CAllGo  *tp  = (CAllGo *)(*t_Protocols.GetItem(j));
            tp->UpdateDeiveState(1);
            tp->devstauts = 1;
        }
    }


}

void	CAllGo::Uninit()
{

}

CAllGo* CreateInstace()
{
    return new CAllGo();
}

void	CAllGo::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, 500);
    if(len < 5)
    {
        //ShowMessage ("Insufficient data length");
        this->GetCurPort()->Clear();
        return;
    }
    ST_INT star = 0;
    for(; star < len; ++star)
    {
        if(pbuf[star] == '=')
            break;
    }
    if(len == star)
    {
        ShowMessage ("Garbled code, clear buffer.");
        this->GetCurPort()->Clear();
        return;
    }
    if(star > 0)
    {
        this->GetCurPort()->ReadBytes(pbuf, star);
    }
    len = this->GetCurPort()->PickBytes(pbuf, 16, 500);
    if(len>=14)
    {
        int endbuf ;
        for(endbuf=0; endbuf<16; endbuf++)
        {
            if(pbuf[endbuf]=='#')
                break;
        }
        len = endbuf+1;
//	    if(pbuf[0]=='='){
        this->GetCurPort()->ReadBytes(pbuf, len);
        readed = len;
        return;
        /*	    }
        	    else{
                    ShowMessage("pbuf formate error");
                    ShowRecvFrame(pbuf,len);
                    this->GetCurPort()->Clear();
                    return;
        	    }*/
    }

}

ST_BOOLEAN	CAllGo::OnSend()
{
    if(m_devSign == AG_WSD)  //温湿度
    {
        time(&Newcurtime);
/*        if(difftime(Newcurtime,oldcurtime)<60*10) //
        {
            if(m_isInit)
                return 1;
        }
*/
        switch(m_rIndex++)
        {
        case 0:
            ReadData(0);
            break;
        case 1:
            ReadData(1);
            break;
        case 2:
            ReadData(2);
            break;
        case 3:
            ReadData(3);
//            time(&oldcurtime);
//            m_isInit = true;
            break;
        default:
            ReadData(0);
            break;
        }
        if(m_rIndex >3) m_rIndex = 0;
    }
    else if(m_devSign == AG_HWYK)    //红外遥控
    {
        if(this->HasTask() && this->GetTask(&m_curTask))
        {
            if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
            {

                if(m_curTask.taskCmdCode == 1)
                {
                    ShowMessage("Enter YK  group. ");
                    SendYK(m_curTask.taskAddr);
                }

            }
            return true;
        }
    }
    else if(m_devSign == AG_ZNKG)  //智能开关
    {
        if(this->HasTask() && this->GetTask(&m_curTask))
        {
            if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
            {

                if(m_curTask.taskCmdCode == 1)
                {
                    ShowMessage("Enter YK  group. ");
                    SendKG(m_curTask.taskAddr,m_curTask.taskValue);
                }

            }
            return true;
        }

        ReadStatus();
    }
    else if(m_devSign == AG_ZNCLKZ) //智能窗帘控制
    {
        if(this->HasTask() && this->GetTask(&m_curTask))
        {
            if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
            {

                if(m_curTask.taskCmdCode == 1)
                {
                    ShowMessage("Enter YK  group. ");
                    if(m_curTask.taskAddr==3 || m_curTask.taskAddr==4)
                    {
                        sendCLSTOP(m_curTask.taskValue ,m_curTask.taskAddr-2);
                    }
                    else
                    {
                        sendCLKZ(m_curTask.taskValue,m_curTask.taskAddr);
                    }
                    //SendKG(m_curTask.taskValue);
                }
            }
            return true;
        }

    }
    return 1;
}

ST_BOOLEAN	CAllGo::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    char msg[32] = {0};
    Memcpy(msg,pbuf,len);
    ShowMessage(msg);
    t_Protocols = m_pChannel->GetCEngine()->m_pProtocols;
    if(t_Protocols.GetCount()<1)
    {
        return 1;
    }
    for(int j = 0; j < t_Protocols.GetCount(); j++)
    {
        CAllGo  *tp  = (CAllGo *)(*t_Protocols.GetItem(j));
        if(tp != NULL)
        {
            if((tp->GetDevice()->GetDeviceInfo()->Addressex)==NULL)
                continue;

            if(strncmp((const char*)&pbuf[1],(const char*)tp->GetDevice()->GetDeviceInfo()->Addressex,9)==0)
            {
                //ShowMessage("get true protocols");
                tp->processBuf(pbuf);
                return 1;
            }
        }

    }
    ShowMessage("not found this address ,please check config");

    return 1;
}

ST_BOOLEAN	CAllGo::IsSupportEngine(ST_INT engineType)
{

    return 1;
}

void     CAllGo::ReadData(int idex)
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = 'R';
    sendbuf[12] = 0x30+idex;
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);
}

void    CAllGo::processBuf(ST_BYTE* pbuf)
{
    switch(m_devSign)
    {
    case AG_WSD:
    {
        if(pbuf[11] == 'e')
        {
            hum_Inter = (pbuf[12]-0x30)*10+(pbuf[13]-0x30);
        }
        if(pbuf[11] == 'f')
        {
            hum_decimal = (pbuf[12]-0x30);
            humidity = (hum_Inter*10+hum_decimal)*0.1;
            this->UpdateValue(0,humidity);
        }
        if(pbuf[11] == 'g')
        {
            temp_Inter = (pbuf[12]-0x30)*10+(pbuf[13]-0x30);
        }
        if(pbuf[11] == 'h')
        {
            temp_decimal = (pbuf[12]-0x30);
            temperature = (temp_Inter*10+temp_decimal)*0.1;
            this->UpdateValue(1,temperature);
        }
    }
    break;
    case AG_SJ:
    {
        if(pbuf[11] == 'p')
        {
            if(pbuf[12] == '0' ||pbuf[12] == '1')
                this->UpdateValue(0,pbuf[12]-0x30);
            if(pbuf[12] == '2' ||pbuf[12] == '3')
                this->UpdateValue(pbuf[12]-0x30-1,1);
        }
    }
    break;
    case AG_YG:
    {
        if(pbuf[11] == 'p')
        {
            if(pbuf[12] == '0' ||pbuf[12] == '1')
                this->UpdateValue(0,pbuf[12]-0x30);
            if(pbuf[12] == '2' ||pbuf[12] == '3')
                this->UpdateValue(pbuf[12]-0x30-1,1);
        }
    }
    break;
    case AG_HWYK:
    {
        if(pbuf[11] == 'a')
        {
            m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
            Memset(&m_curTask, 0, sizeof(m_curTask));
            return ;
        }
    }
    break;
    case AG_ZNKG:
    {
        if(pbuf[11] == 's')
        {
            /*if(pbuf[12] == '0' ||pbuf[12] == '1')
                this->UpdateValue(0,pbuf[12]-0x30);*/
            ST_BYTE mbyte = pbuf[12] - '0';
            for(int i = 0;i<3;i++)
            {
                this->UpdateValue(i,(mbyte>>i)&0x01);
            }
        }
        else if(pbuf[11] == 'n')
        {
            m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
            Memset(&m_curTask, 0, sizeof(m_curTask));
            return ;
        }
        else if(pbuf[11] == 'm')
        {
            if(pbuf[13] == '0' ||pbuf[13] == '1')
                this->UpdateValue(0,pbuf[13]-0x30);
        }
    }
    break;
    case AG_ZNCLKZ:
    {
        m_curTask.taskResult.resultCode = 0;
        m_curTask.isTransfer = 1;
        Transfer(&m_curTask);
        Memset(&m_curTask, 0, sizeof(m_curTask));
        return ;
    }
    default:
        break;
    }
}

/*
ST_BYTE  CAllGo::BCD2Hex(ST_BYTE byte)
{
    ST_BYTE mb = 0x00;
    if(pbuf[12] >= '0' && pbuf[12] <= '7')
        mb = pbuf[12] - '0';
    return mb;
}*/

void    CAllGo::SendYK(ST_UINT writeAddr)
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = 'A';
    sendbuf[12] = 0x30+writeAddr;
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);

    char msg[32] = {0};
    Memcpy(msg,sendbuf,14);
    ShowMessage(msg);
}

void    CAllGo::ReadStatus()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = 'S';
    sendbuf[12] = 'T';
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);

    char msg[32] = {0};
    Memcpy(msg,sendbuf,14);
    ShowMessage(msg);
}

void    CAllGo::SendKG(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = bIsOn == 1?'N':'F';
    sendbuf[12] = '0' + writeAddr;
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);

    char msg[32] = {0};
    Memcpy(msg,sendbuf,14);
    ShowMessage(msg);
}


void    CAllGo::sendCLKZ(ST_BOOLEAN bIsOn,ST_UINT writeAddr)  //智能窗帘控制
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = bIsOn == 1?'P':'Q';
    sendbuf[12] = writeAddr==1?'1':'2';
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);

    char msg[32] = {0};
    Memcpy(msg,sendbuf,14);
    ShowMessage(msg);

}

void    CAllGo::sendCLSTOP(ST_BOOLEAN bIsOn,ST_UINT writeAddr)
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

    ST_BYTE sendbuf[24];
    sendbuf[0] = '=';
    sendbuf[1] = info.Addressex[0];
    sendbuf[2] = info.Addressex[1];
    sendbuf[3] = info.Addressex[2];
    sendbuf[4] = info.Addressex[3];
    sendbuf[5] = info.Addressex[4];
    sendbuf[6] = info.Addressex[5];
    sendbuf[7] = info.Addressex[6];
    sendbuf[8] = info.Addressex[7];
    sendbuf[9] = info.Addressex[8];
    sendbuf[10] = ',';
    sendbuf[11] = 'S';
    sendbuf[12] = writeAddr==1?'1':'2';
    sendbuf[13] = '#';
    //ShowMessage((char *)sendbuf);
    this->Send(sendbuf, 14);

    char msg[32] = {0};
    Memcpy(msg,sendbuf,14);
    ShowMessage(msg);
}








