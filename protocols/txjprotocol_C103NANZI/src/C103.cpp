#include "C103.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


C103::C103():
_sendstate(0),_callalled(false),m_nStart(false),_acked(false)
{
    //ctor
    InitUDPClient();
}

C103::~C103()
{
    //dtor
}

void C103::Init()
{
    _lastcalltime = clock();

}

void C103::Uninit()
{
    close(fd);
}


ST_BOOLEAN	C103::IsSupportEngine(ST_INT engineType)
{
    return engineType == EngineBase::ENGINE_FULLING;
}


C103* CreateInstace()
{
    return new C103();
}

ST_BOOLEAN	C103::OnSend()
{
    Thread::SLEEP(50);
    m_bTask = false;
    // 先处理遥控任务
    if(this->HasTask() && this->GetTask(&m_curTask))
    {
        if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
           if(m_curTask.taskCmdCode == 0) {
                YKSelect(m_curTask);
                m_bTask = true;
            }
           else if(m_curTask.taskCmdCode == 1) {
                YKExecut(m_curTask);
                m_bTask = true;
            }
           else if(m_curTask.taskCmdCode == 2) {
                YKCancel(m_curTask);
                m_bTask = true;
            }
        }
        return true;
    }

    // 循环发送
    switch(_sendstate++)
    {
        case 0x00:
            break;
        // 总召
        case 0x01:{
                clock_t _curtime = clock();
                if((abs(_curtime - _lastcalltime) > 30*60 * CLOCKS_PER_SEC) || (!m_nStart))
                    _callalled = false;
                if(!_callalled)
                {
                   CallAll_ASDU7();
                   _callalled = true;
                   _lastcalltime = clock();
                }
            }break;
        /*
        // 遥信组08
        case 0x02:  AskGroupData(0x08); break;
        // 遥测组07
        case 0x03:  AskGroupData(0x07); break;
        // 保护测量量组06
        case 0x04:  AskGroupData(0x06); break;
        // 对时 函数还没写完
        case 0x05:  Clocksync_ASDU6();  break;
        */
        default:    _sendstate = 0;     break;
    }
    return true;
}


void C103::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
    char psg[20];
    sprintf(psg,"fd : %d||ret :%d",fd,ret);
    ShowMessage(psg);
/////////////////////////////////////////////////////////////
    readed = 0;
    if (!this->IsOpened()){
        m_nStart = false;
        return;
    }

    clock_t now = clock();
    if ( (!_acked) || (abs(now - _lastreadtime) > 30 * CLOCKS_PER_SEC) ){
        Thread::SLEEP(100);
        CreateUDP(NULL);
    }


    if (!this->GetCurPort())
        return;

    ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

    ST_INT len = this->GetCurPort()->PickBytes(pbuf,8,interval);
    if (len >= 8)
    {
        ST_INT star = 0;
        for(; star < len; star++)
        {
            // 只处理ASDU10格式的报文
            if (pbuf[star] == 0x0A)
                break;
        }
        if (star == len)
        {
            ShowMessage("该报文都是乱码，丢弃该报文！");
            this->GetCurPort()->Clear();
            return;
        }

        if (star > 0)
        {
            ShowMessage("丢弃乱码部分！");
            this->GetCurPort()->ReadBytes(pbuf,star);
        }

        len = this->GetCurPort()->PickBytes(pbuf,8,interval);
        if (len >= 8)
        {
            ST_INT nlen = len;
            for (ST_INT g_no = 0; g_no < pbuf[7]; g_no++)
            {
                nlen = len + 6;
                len  = this->GetCurPort()->PickBytes(pbuf,nlen,interval);
                nlen = len + pbuf[len-1] * pbuf[len-2];
                len  = this->GetCurPort()->PickBytes(pbuf,nlen,interval);
                if (len != nlen)
                {
                    ShowMessage("报文长度不足，继续接收报文");
                    return;
                }
            }
            this->GetCurPort()->ReadBytes(pbuf,len);
            if (len == nlen)
                readed = len;
            else
            {
                ShowMessage("Error mes.");
                this->GetCurPort()->Clear();
            }
        }
    }
}


ST_BOOLEAN	C103::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
    _acked = true;
    _lastreadtime = clock();

    if (m_bTask)
    {
        if (!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
            m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
            memset(&m_curTask,0,sizeof(m_curTask));
            return true;
        }
    }

    switch(pbuf[0]){ // TYP
        case 0x0A : AnalysisASDU10(pbuf,len); break;
        default:break;
    }

    return true;

}


void C103::YKSelect(ProtocolTask & task)
{
    ST_BYTE sendbuf[15] = {0};
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x81;
    sendbuf[2] = 0x28;
    sendbuf[3] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[4] = 0xfe;
    sendbuf[5] = 0xf9;
    sendbuf[6] = 0x00;
    sendbuf[7] = 0x01;
    // GIN
    sendbuf[8] = 0x0B;                 // 组号 11
    sendbuf[9] = task.taskAddr;      // 不同条目号 代表不同开关 不知道固定的是哪个开关
    // KOD
    sendbuf[10] = 0x01;
    // GDD
    sendbuf[11] = 0x09;
    sendbuf[12] = 0x01;
    sendbuf[13] = 0x01;
    // GID
    sendbuf[14] = (task.taskValue == 2 ? 0x02 : 0x01);
    this->Send(sendbuf,15);
}


void C103::YKExecut(ProtocolTask & task)
{
    ST_BYTE sendbuf[15] = {0};
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x81;
    sendbuf[2] = 0x28;
    sendbuf[3] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[4] = 0xfe;
    sendbuf[5] = 0xfA;
    sendbuf[6] = 0x00;
    sendbuf[7] = 0x01;
    // GIN
    sendbuf[8] = 0x0B;
    sendbuf[9] = task.taskAddr;
    // KOD
    sendbuf[10] = 0x01;
    // GDD
    sendbuf[11] = 0x09;
    sendbuf[12] = 0x01;
    sendbuf[13] = 0x01;
    // GID
    sendbuf[14] = (task.taskValue == 2 ? 0x02 : 0x01);
    this->Send(sendbuf,15);
}


void C103::YKCancel(ProtocolTask & task)
{
    // nothing
}


void C103::Send_ADSU21(ASDU21 & buf)
{
    ST_BYTE sendbuf[1024] = {0};
    sendbuf[0] = 0x15;
    sendbuf[1] = 0x81;
    sendbuf[2] = buf.cot;
    sendbuf[3] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[4] = buf.fun;
    sendbuf[5] = buf.inf;
    sendbuf[6] = buf.rii;
    memcpy(&sendbuf[7],buf.unitbuf,buf.unitbuf_size);
    this->Send(sendbuf,7+buf.unitbuf_size);
}


void C103::AskGroupData(ST_BYTE GroupNo)
{
    ASDU21 SBUF;
    SBUF.cot = 0x2A;
    SBUF.fun = 0xFE;
    SBUF.inf = 0xF1;
    SBUF.rii = 0x00;
    SBUF.nog = 0x01;
    SBUF.unitbuf_size = 0x03;
    ST_BYTE buf[3] = {0x00,0x00,0x01};
    0[buf] = GroupNo;
    SBUF.unitbuf = buf;
    Send_ADSU21(SBUF);
}


void C103::CallAll_ASDU21()
{
    ST_BYTE sendbuf[8] = {0};
    sendbuf[0] = 0x15;
    sendbuf[1] = 0x81;
    sendbuf[2] = 0x09;
    sendbuf[3] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[4] = 0xfe;
    sendbuf[5] = 0xf5;
    sendbuf[6] = 0x00;
    sendbuf[7] = 0x00;
    this->Send(sendbuf,8);
}


void C103::CallAll_ASDU7()
{
    ST_BYTE sendbuf[7] = {0};
    sendbuf[0] = 0x07;
    sendbuf[1] = 0x81;
    sendbuf[2] = 0x09;
    sendbuf[3] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[4] = 0xff;
    sendbuf[5] = 0x00;
    sendbuf[6] = 0x00;
    //sendbuf[6] = 0x04;      // SCN 0-255 示例写着04
    // 由主站给出，子站应答报文中包含与本扫描序号（SCN）一致的返回码（RII）
    this->Send(sendbuf,7);
}

// to be countie
void C103::Clocksync_ASDU6()
{
    ST_BYTE sendbuf[100] = {0};
    sendbuf[0] = 0x06;
    sendbuf[1] = 0x81;
    sendbuf[2] = 0x08;
    sendbuf[3] = 0xff;
    sendbuf[4] = 0xff;
    sendbuf[5] = 0x00;
    /*
    sendbuf[6] = ;
    sendbuf[7] = ;
    sendbuf[8] = ;
    sendbuf[9] = ;
    sendbuf[10] = ;
    sendbuf[11] = ;
    sendbuf[12] = ;
    */
    this->Send(sendbuf,13);
    // 时标不知道怎么写
}


void C103::AnalysisASDU10(ST_BYTE * buf,ST_INT & cnt)
{
    ST_BYTE NGD = buf[7];
    // 组号开始
    ST_BYTE * dptr = buf + 8;
    for (ST_INT i=0; i < NGD; i++, dptr += (dptr[4] * dptr[5] + 6))
    {
        switch(*dptr){ // 组号
            case 0x07:{ // YC
                if (dptr[3] == 0x0c) { //数据类型
                    ST_UINT16 value = dptr[6] + (dptr[7] & 0x1F) * 256;
                    this->UpdateValue(dptr[1]+9999,(float)value);
                }
              }break;
            case 0x08:{ // YX
                if (dptr[3] == 0x09){ //双点信息
                    ST_BOOLEAN value = (dptr[6] == 2 ? true:false);
                    this->UpdateValue(dptr[1]-1,value);
                }
              }break;
            case 0x18:{ // SOE  应该是GDD数据类型为18（带CP32Time2a时标的报文）
                ST_BOOLEAN value = (6[dptr] == 2 ? true:false);
                ST_INT32 addr =  dptr[1] - 1;
				TransferEx (value, addr, dptr[10], dptr[9], dptr[7] + dptr[8] * 256);
                this->UpdateValue(addr, value);
              }break;
            case 0x0E:{ // YK
              }break;
            case 0x06:{ // PYC
              }break;
            case 0x0B:{ // 压板
              }break;
            case 0x0A:{ // 电度 文件没用上不知道其
              }break;
            default: break;
        }
    }
}


void C103::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
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

void    C103::InitUDPClient()
{
    fd = socket(AF_INET,SOCK_DGRAM,0);
    if (fd == -1){
        ShowMessage("UDP_Scoket create sucess.");
        return;
    }
    //准备地址
     struct sockaddr_in addr = {};
     addr.sin_family = AF_INET;//ipv4
     addr.sin_port = htons(5577);//端口号
     addr.sin_addr.s_addr = inet_addr("192.168.1.177");//我的ip地址
     //绑定
     ret=0;
//     ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
     if (0 > ret)
     {
          ShowMessage("bind successful");
     }



}

void C103::CreateUDP(void* px)
{
/*  1.创建clientSocket
    2.设置服务器地址 serverAddr
    3.可选 设置clientAddr并和clientSocket（一般不用绑定）
    4.进行发送操作 sendto
    5.关闭clientSocket        */
/*
    // 1. create clientSocket
    int fd = socket(AF_INET,SOCK_DGRAM,0);
    if (fd == -1){
        ShowMessage("UDP_Scoket create sucess.");
        return;
    }
*/
    // 获取扩展地址
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    ST_INT i = 0;
    for (; i<256; i++)
    {
        if (info.Addressex[i] == '/')
            break;
    }
    ST_CHAR _addr[i+1];
    memcpy(_addr, info.Addressex, i);
    _addr[i] = '\0';
    const char* addrp = _addr;

    ST_INT portlen = i+1;
    for (; portlen < 256; portlen++)
    {
        if (info.Addressex[portlen] == '/')
            break;
    }

    ST_INT16 myport = 0;
    switch(portlen-i-1){
        case 1: myport = info.Addressex[i+1] - '0'; break;
        case 2: myport = (info.Addressex[i+1] - '0')*10 + (info.Addressex[i+2] - '0'); break;
        case 3: myport = (info.Addressex[i+1] - '0')*100 + (info.Addressex[i+2] - '0')*10 + (info.Addressex[i+3] - '0'); break;
        case 4: myport = (info.Addressex[i+1] - '0')*1000 + (info.Addressex[i+2] - '0')*100 + (info.Addressex[i+3] - '0')*10 + (info.Addressex[i+4] - '0'); break;
        case 5: myport = (info.Addressex[i+1] - '0')*10000 + (info.Addressex[i+2] - '0')*1000 + (info.Addressex[i+3] - '0')*100 + (info.Addressex[i+4] - '0')*10 +  (info.Addressex[i+5] - '0'); break;
        default: break;
    }
//    ShowMessage(addrp);

    //  2. 目标服务器地址
    struct sockaddr_in addr_to;
    addr_to.sin_family = AF_INET;
    addr_to.sin_port   = htons(myport);
    addr_to.sin_addr.s_addr = inet_addr(addrp);

    ST_BYTE sendbuf[41] = {0};
    sendbuf[0] = 0xFE;
    int numbytes;
    if ((numbytes = sendto(fd, sendbuf, sizeof(sendbuf), 0, (struct sockaddr*)&addr_to, sizeof(struct sockaddr))) == -1){
        ShowMessage("UDP Send fail");
        return;
    }
    else
        ShowMessage("UDP Send sucess");
//    close(fd);
}
