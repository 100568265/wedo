#ifndef PORTTCPCLIENTBASE_H_INCLUDE
#define PORTTCPCLIENTBASE_H_INCLUDE

#include "DataCache.h"
#include "PortBase.h"
#include "PortTcp.h"


class MonitorClient;

class PortTcpClientBase:public PortBase,public PortTcp
{
public:
    PortTcpClientBase(Channel *channel,PortInfo *portInfo);
    ~PortTcpClientBase();
    ST_VOID					Init();
    ST_VOID					Uninit();
    ST_VOID					Open();
    ST_VOID					Close();
    ST_BOOLEAN				Send(ST_UINT64 portAddr,ST_BYTE *pBuf,ST_UINT size);
    ST_VOID					Recv();
protected:
    ST_SOCKET				client_socket_fd ;
    struct	sockaddr_in		m_Server_addr;
public:
	MonitorClient			*m_client;
};

#endif // PORTTCPCLIENTBASE_H_INCLUDE
