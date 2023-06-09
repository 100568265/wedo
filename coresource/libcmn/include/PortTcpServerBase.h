#ifndef PORTTCPSERVERBASE_H_INCLUDE
#define PORTTCPSERVERBASE_H_INCLUDE

#include "datatype.h"
#include "sysmalloc.h"
#include "PortBase.h"
#include "PortTcp.h"

#include <sys/epoll.h>

#define MAX_EPOLLSIZE 128

class PortTcpServerBase:public PortBase,public PortTcp
{
public:
    PortTcpServerBase(Channel *channel,PortInfo *portInfo);
    virtual ~PortTcpServerBase();
    ST_VOID 					Init();
    ST_VOID 					Uninit();
    ST_VOID 					Open();
    ST_VOID 					Close();
	ST_VOID 					Recv();
	ST_VOID 					Stop();
    ST_BOOLEAN 					IsOpened();
    ST_BOOLEAN 					Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size);
	ST_VOID						CloseClient(ST_SOCKET &sock_fd);
protected:
    virtual ST_INT 				Listen();
    virtual ST_INT 				Accept();
    virtual ST_INT 				ProcessWrite(ST_SOCKET sock_fd,ST_BYTE *buf,ST_UINT size);
	virtual ST_INT				ProcessRead(ST_SOCKET sock_fd);
#ifdef _WIN32
    static ST_UINT32 __stdcall	AcceptThread(ST_VOID *param);
#elif __unix__
    static ST_VOID				*AcceptThread(ST_VOID *param);
#endif
protected:
#ifdef __linux__
    ST_INT						m_epoll_read_fd,m_REvents;
    struct epoll_event			m_ReadEvents[2],ev;
#endif
    Thread						m_ClientThread;
    ST_SOCKET 					m_Server_listen_fd;
    ST_SOCKET 					m_Client_connected_fd;
	struct sockaddr_in			m_Server_addr,m_Client_addr;
    socklen_t					m_SockLen ;
};

#endif // PORTTCPSERVERBASE_H_INCLUDE
