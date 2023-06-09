#ifndef PORTTCPSERVER_LINUX_H
#define PORTTCPSERVER_LINUX_H

#include "datatype.h"
#include "PortBase.h"
#include "PortTcp.h"
#include "sysmalloc.h"

#include <sys/epoll.h>

#include <map>

class TcpSocketSharer;

class PortTcpServer : public PortBase, protected PortTcp
{
    friend class TcpSocketSharer;
public:
    enum {
        MAXEPOLLSIZE = 128,
        MAXLISTENQUE = 1024
    };
    enum { MAXCLIENTS = 1024 };

	class SockInfo {
    public:
        SockInfo() : socketfd(-1) {
            Memset(&addr_in, 0, sizeof(addr_in));
        }
        ST_SOCKET socketfd;
        struct sockaddr_in addr_in;
    };
    typedef std::map<ST_SOCKET, SockInfo>::iterator ClientItor;

    PortTcpServer(Channel *channel, PortInfo *portInfo);
    virtual ~PortTcpServer();
    ST_VOID 					Init();
    ST_VOID 					Uninit();
    ST_VOID 					Open();
    ST_VOID 					Close();
	ST_VOID						Close(ST_UINT64 port);
    ST_VOID                     Work();
    ST_VOID                     Stop();
	ST_VOID 					Recv();
    ST_BOOLEAN 					IsOpened();
    ST_BOOLEAN 					Send(ST_UINT64 portAddr, ST_BYTE *buf, ST_UINT size);
    ST_BOOLEAN 					Send(ST_UINT64 portAddr, char *buf, ST_UINT size);

protected:
    virtual ST_INT 				ProcessRead(ST_SOCKET sock_fd);
    virtual ST_INT 				ProcessWrite(ST_SOCKET sock_fd, ST_BYTE *buf, ST_UINT size);

    bool                        AddClient  (ST_SOCKET, struct sockaddr_in&); ///< 添加客户fd, 失败或已存在返回false
    ST_VOID						CloseClient(ST_SOCKET, bool remove = true);

    ST_VOID						CloseAllClient();

	ClientItor					FindClient (const struct sockaddr_in&);
    ClientItor                  FindClient (const struct in_addr&);

    static TcpSocketSharer&     GetSharer();

    inline bool                 hasRemoteAddr ();                ///< 对方地址是否固定
protected:
    struct epoll_event          m_readEvents[MAXEPOLLSIZE];
private:
    ST_SOCKET                   m_server_fd;
    ST_INT32                    m_epollfd;
    struct sockaddr_in          m_server_addr;

    std::map<ST_SOCKET, SockInfo>   m_clients;
};

#endif // PORTTCPSERVER_LINUX_H
