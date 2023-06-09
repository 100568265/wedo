#ifndef PORTCAN_H
#define PORTCAN_H
#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "PortTcp.h"
#include "PortBase.h"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

class PortCan:public PortBase
{
public:
    PortCan(Channel *channel,PortInfo *portInfo);   //Can通道构造函数
    virtual ~PortCan();     //通道析构函数
    ST_VOID Init();
    ST_VOID Uninit();
    ST_VOID Open();
    ST_VOID Close();
    ST_BOOLEAN IsOpened();
    ST_BOOLEAN Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size);
    ST_BOOLEAN Send(ST_UINT64 portAddr,char *buf,ST_UINT size);
    ST_VOID Recv();
    ST_SOCKET   m_Can_socket_fd;    //can套接字
    struct    sockaddr_can addr;
    struct    ifreq ifr;
    struct    can_frame frame;
    struct    can_frame status;
    int family;
protected:



};
#endif //PORTCAN_H_INCLUDED
