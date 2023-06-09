#ifndef PORTUDP_H
#define PORTUDP_H

#include "PortTcp.h"

class PortUdp:public PortBase,public PortTcp
{
public:
    PortUdp(Channel *channel,PortInfo *portInfo);
    virtual ~PortUdp();
    ST_VOID 			Init();
    ST_VOID 			Uninit();
    ST_VOID 			Open();
    ST_VOID 			Close();
    ST_BOOLEAN			IsOpened();
    ST_BOOLEAN			Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size);
    ST_VOID				Recv();
protected:
    ST_SOCKET			m_Udp_socket_fd ;
    socklen_t			m_Addr_local_len,m_Addr_remote_len;
    struct sockaddr_in	m_Local_addr,m_Remote_addr;
};

#endif // PORTUDP_H
