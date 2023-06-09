#include "PortCan.h"
#include "Channel.h"
#include <string>
#include <fstream>

string port;
int address;
PortCan::PortCan(Channel *channel,PortInfo *portInfo):PortBase(channel,portInfo)
{
    m_Can_socket_fd = -1;
}

//Can通道析构函数
PortCan::~PortCan()
{
    Stop();
    Uninit();
}

ST_VOID PortCan::Init()
{
    if(m_Inited)
        return;
    //设置波特率
    /*if(m_pPortInfo->PortName == "can"){
        if(m_pPortInfo->PortNum == 0){
            port = "can0";
                //if(m_pPortInfo->BaudRate ==250000){波特率固定为250K
                system("ifconfig can0 down");
                system("ip link set can0 type can bitrate 250000");
                system("ifconfig can0 up");
                m_pLogger->LogDebug("%s%d opened",m_pPortInfo->PortName,m_pPortInfo->PortNum);

        }
        else if(m_pPortInfo->PortNum == 1){
            port = "can1";
            if(m_pPortInfo->BaudRate ==250000){
                system("ifconfig can1 down");
                system("ip link set can1 type can bitrate 250000");
                system("ifconfig can1 up");
                m_pLogger->LogDebug("%s%d opened",m_pPortInfo->PortName,m_pPortInfo->PortNum);
            }
        }
    }*/
}

ST_VOID PortCan::Uninit()
{
    if(!m_Inited)
        return;
    Close();
    m_Inited = false;
}





ST_VOID PortCan::Open()
{
    if(m_IsOpened)
        return;
    m_Mutex.Lock();
    if(-1!=m_Can_socket_fd)
    {
        close(m_Can_socket_fd);
    }

    if(m_pPortInfo->PortNum == 0)
    {
        system("ifconfig can0 down");
        system("ip link set can0 type can bitrate 250000");
        system("ifconfig can0 up");
    }
    else
    {
        system("ifconfig can1 down");
        system("ip link set can1 type can bitrate 250000");
        system("ifconfig can1 up");
    }
    //创建can套接字
    if((m_Can_socket_fd = socket(PF_CAN,SOCK_RAW,CAN_RAW))==-1)
    {
        m_Mutex.UnLock();
    }
    if(m_pPortInfo->PortNum ==0)
    {
        strcpy(ifr.ifr_name,"can0");
        m_pLogger->LogDebug("%s binded to socket = can0?",ifr.ifr_name);
    }
    else
    {
        strcpy(ifr.ifr_name,"can1");
        m_pLogger->LogDebug("%s binded to socket = can1?",ifr.ifr_name);
    }

    ioctl(m_Can_socket_fd, SIOCGIFINDEX, &ifr); //指定编号为can0的设备，获取设备索引
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    //将套接字与can0绑定
    if(bind(m_Can_socket_fd,(struct sockaddr*)&addr,sizeof(addr))<0)
    {
        m_Mutex.UnLock();
    }
    m_pLogger->LogDebug("bind socket with %d(socket value before bind())",m_Can_socket_fd);
    m_Mutex.Notify();
    m_IsOpened = true;
    //m_pChannel->GetCEngine()->OnConnect(m_Can_socket_fd,m_Can_socket_fd);

}


ST_VOID PortCan::Close()
{
    if(-1!=m_Can_socket_fd)
    {
        close(m_Can_socket_fd);
    }
    //m_pChannel->GetCEngine()->OnDisconnect(m_Can_socket_fd,m_Can_socket_fd);
}

ST_BOOLEAN PortCan::IsOpened()
{
    return m_IsOpened;
}
ST_BOOLEAN PortCan::Send(ST_UINT64 portAddr, char *buf, ST_UINT size)
{

}

ST_BOOLEAN PortCan::Send(ST_UINT64 portAddr, ST_BYTE *buf, ST_UINT size)
{
    ST_INT nLeft=size, sendLen=0;
    if(IsOpened())
    {
        while(nLeft>0)
        {
            for(int i = 0; i<nLeft-2; i++)
            {
                frame.data[i] = *(buf+i+2); //把buf里的数据包装成can帧
                m_pLogger->LogDebug("frame[%d] = %d",i,frame.data[i]);
            }
            //memcpy();
            address = (*buf);
            frame.can_id = (*buf)<<3|5;
            frame.can_dlc =*(buf+1);
            sendLen = write(m_Can_socket_fd, &frame, sizeof(frame));
            m_pLogger->LogDebug("send frame: sendLen = %d, socket = %d, &frame = %d, frame.can_id = %d",sendLen,m_Can_socket_fd,&frame,frame.can_id);   //测试发送缓冲区
            if(sendLen>0)
            {
                buf+=sendLen;
                nLeft-=sendLen;
            }
            else
            {
                m_pLogger->LogDebug("can write error, errno:%d, desc:%s",errno,strerror(errno));
                return false;
            }
        }
        return true;
    }
    else
    {
        m_pLogger->LogDebug("can write error,m_IsOpened=false.");
        return false;
    }

}

//接收数据
ST_VOID PortCan::Recv()
{
    if(!m_IsOpened)
    {
        Thread::SLEEP(1000);
        return;
    }
    FD_ZERO(&m_ReadSet);
    FD_SET(m_Can_socket_fd,&m_ReadSet);
    if(select(m_Can_socket_fd+1,&m_ReadSet,NULL,NULL,NULL)>0)
    {
        if(FD_ISSET(m_Can_socket_fd, &m_ReadSet))
        {
            //can frame receive function
            int readLen = read(m_Can_socket_fd,&status,sizeof(status));
            if (status.can_id == ((address<<3)|5)) // the only frame
            {
                m_pLogger->LogDebug("address = %d",address);
                for (int i = 0; i < status.can_dlc; i++)
                {
                    m_PortBuf[i] = status.data[i];
                    //m_pLogger->LogDebug("m_PortBuf[%d] = %d,AAAcan_id=%d",i,m_PortBuf[i],status.can_id);
                }
                m_pLogger->LogDebug("total bytes = %d",m_PortBuf[1]);
                m_portTask.Write(m_PortBuf,m_PortBuf[1]);
                m_portTask.PortDstAddr = -1;
                m_portTask.LocalChannelID = m_pChannel->GetLocalChannelID();
                m_pChannel->GetCEngine()->ReadTask(&m_portTask);
                return;
            }
            else if (status.can_id == ((address<<3)|1)) // first frame of N frames
            {
                int k = 0;
                for (int i = 0; i < status.can_dlc; i++)
                {
                    m_PortBuf[i] = status.data[i];
                    //m_pLogger->LogDebug("m_PortBuf[%d] = %d,can_id=%d,can_dlc=%d",i,m_PortBuf[i],status.can_id,status.can_dlc);
                }
                k+=status.can_dlc;
                int readLen2 = 0;
                while((readLen2 = read(m_Can_socket_fd, &status, sizeof(status))>0 && status.can_id==((address<<3)|2)))
                {
                    readLen += readLen2;
                    for (int i = 0; i < status.can_dlc; i++)
                    {
                        m_PortBuf[i+k] = status.data[i];
                        //m_pLogger->LogDebug("m_PortBuf[%d] = %d,can_id=%d,can_dlc=%d",i+k,m_PortBuf[i+k],status.can_id,status.can_dlc);
                    }
                    k+=status.can_dlc;
                }


                readLen += readLen2;
                for (int i = 0; i < status.can_dlc; i++)    //last frame
                {
                    m_PortBuf[i+k] = status.data[i];
                    //m_pLogger->LogDebug("m_PortBuf[%d] = %d,can_id=%d,can_dlc=%d",i+k,m_PortBuf[i+k],status.can_id,status.can_dlc);

                }
                k+=status.can_dlc;

                m_pLogger->LogDebug("total bytes = %d",m_PortBuf[1]);
                m_portTask.Write(m_PortBuf,m_PortBuf[1]);
                m_portTask.PortDstAddr = -1;
                m_portTask.LocalChannelID = m_pChannel->GetLocalChannelID();
                m_pChannel->GetCEngine()->ReadTask(&m_portTask);
                return;
            }

            else if(readLen < 0 && (errno==EAGAIN || errno==EINTR))
            {
                return;
            }
            else
            {
                m_pLogger->LogDebug("can%d recv error,errno:%d,desc:%s",m_pPortInfo->PortNum,errno,strerror(errno));
            }
        }
        else
        {
            return;
        }
    }
    m_pLogger->LogDebug("can%d select error, errno:%d, desc:%s",m_pPortInfo->PortNum, errno, strerror(errno));

}






