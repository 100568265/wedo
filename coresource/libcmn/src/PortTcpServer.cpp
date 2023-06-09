//#include "stdafx.h"
#include "PortTcpServer.h"
#include "sysmalloc.h"
#include "Channel.h"

#include "wedo/thread/mutex.h"

#include <map>
using namespace std;

#define slogger SysLogger::GetInstance

static bool sockaddr_equal (const struct sockaddr_in& lhs, const struct sockaddr_in& rhs)
{
    return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr
        && lhs.sin_port        == rhs.sin_port
        && lhs.sin_family      == rhs.sin_family;
}

class TcpSocketSharer
{
public:
    typedef std::multimap<ST_SOCKET, PortTcpServer*> Type;
    typedef std::multimap<ST_SOCKET, PortTcpServer*>::iterator Iterator;

    TcpSocketSharer();                  ///< TcpSocketSharer类构造函数
    virtual ~TcpSocketSharer();         ///< TcpSocketSharer类析构函数

    ST_VOID     ThreadWork();               ///< 启动线程, 使accept客户端请求
    ST_VOID     ThreadStop();               ///< 停止线程
    inline bool isThreadWork() const;

    int         Open  (PortTcpServer * ptr);   ///< 绑定并侦听本机端口 /// @see TcpSocketSharer::Close()
    void        Close (PortTcpServer * ptr);   ///< 关闭PortTcpServer类的所有客户accept, 和关联的侦听端口

    void        Accept();

    Iterator find (struct sockaddr_in& addrin)
    {
        for (Iterator it = _mmap.begin(); it != _mmap.end(); ++it)
        {
            if (sockaddr_equal(it->second->m_server_addr, addrin))
                return it;
        }
        return _mmap.end();
    }

    Iterator find (ST_SOCKET sockfd, PortTcpServer * ptr) {
        std::pair<Iterator, Iterator> range = _mmap.equal_range (sockfd);
        for (Iterator it = range.first; it != range.second; ++it) {
            if (it->second == ptr)
                return it;
        }
        return _mmap.end();
    }

    static ST_VOID * acceptProc (void * param);
private:
    void        CloseAll ();

    bool               _isThreadWork;
    Thread             _acceptThread;
    wedo::mutex        _mutex;

    ST_INT32           _epollfd;
    struct epoll_event _events[PortTcpServer::MAXEPOLLSIZE];
    Type               _mmap;
};

TcpSocketSharer::TcpSocketSharer():
_isThreadWork(false)
{
    _epollfd = epoll_create(PortTcpServer::MAXEPOLLSIZE);
}

TcpSocketSharer::~TcpSocketSharer()
{
    this->ThreadStop();
    this->CloseAll ();
}

ST_VOID TcpSocketSharer::ThreadWork()
{
    if(!_isThreadWork) {
        _acceptThread.Start(acceptProc, this, true);
        _isThreadWork = true;
    }
}

ST_VOID TcpSocketSharer::ThreadStop()
{
    if(_isThreadWork) {
        _isThreadWork = false;
        _acceptThread.Stop(1000);
    }
}

bool TcpSocketSharer::isThreadWork() const
{
    return _isThreadWork;
}

int TcpSocketSharer::Open(PortTcpServer * ptr)
{
    if (!ptr) return -1;

    wedo::lock_guard grd (_mutex);
    // 寻找库中有无此地址，此端口
    Iterator it = this->find (ptr->m_server_addr);
    if (it != _mmap.end()) {
        if (this->find (it->first, ptr) != _mmap.end())
            return 0;
        ptr->m_server_fd = it->second->m_server_fd;
        _mmap.insert (make_pair(ptr->m_server_fd, ptr));
    }
    else {
        ST_SOCKET socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(-1 == socketfd)
            return -1;
        ptr->SetNonBlocking(socketfd);   //设置sockfd为非阻塞模式
        ptr->SetReuseAddr(socketfd, 1);  //端口复用  用于服务端绑定同一端口
        ptr->SetLinger(socketfd, 1, 0);  //延迟关闭连接

        if (0 > bind(socketfd, (struct sockaddr*)&(ptr->m_server_addr), sizeof(ptr->m_server_addr)))
        {
            slogger()->LogWarn("%s tcpserver bind error, errno: %d, desc: %s",
                ptr->GetChannel()->GetChannelInfo()->ChannelName, errno, strerror(errno));
            return -1;
        }
        if (0 > listen(socketfd, PortTcpServer::MAXLISTENQUE)) {
            slogger()->LogWarn("%s tcpserver listen error, errno: %d, desc: %s",
                ptr->GetChannel()->GetChannelInfo()->ChannelName, errno, strerror(errno));
            return -1;
        }
        //注册epoll需要监听的事件
        struct epoll_event event;
        event.events = EPOLLIN;// | EPOLLET;
        event.data.fd = socketfd;
        if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, socketfd, &event) < 0) {
            return -1;
        }
        ptr->m_server_fd = socketfd;
        _mmap.insert (make_pair(socketfd, ptr));
    }
    return 0;
}
/*
EPOLL_CTL_ADD  // 注册目标fd到epfd中，同时关联内部event到fd上

EPOLL_CTL_MOD // 修改已经注册到fd的监听事件

EPOLL_CTL_DEL // 从epfd中删除/移除已注册的fd，event可以被忽略，也可以为NULL

*/
void TcpSocketSharer::Close(PortTcpServer * ptr)
{
    if(!ptr) return;
    if (ptr->m_server_fd == -1) return;

    wedo::lock_guard grd (_mutex);

    Iterator it = this->find (ptr->m_server_fd, ptr);
    if (it != _mmap.end()) {
        ptr->CloseAllClient();

        if (1 == _mmap.count (ptr->m_server_fd)) {
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = ptr->m_server_fd;
            epoll_ctl(_epollfd, EPOLL_CTL_DEL, ptr->m_server_fd, &event);

            close (ptr->m_server_fd);
            ptr->m_server_fd = -1;
        }
        _mmap.erase (it);
        ptr->m_server_fd = -1;
    }
}

void TcpSocketSharer::CloseAll()
{
    for (Iterator it = _mmap.begin(); it != _mmap.end();/**/) {
        Iterator temp = it++;
        this->Close (temp->second);
    }
    _mmap.clear ();
}

void TcpSocketSharer::Accept ()
{
    if (_epollfd == -1) {
        Thread::SLEEP(200); return;
    }
    ST_INT32 evCount = epoll_wait(_epollfd, this->_events, PortTcpServer::MAXEPOLLSIZE, -1);
    if (evCount == -1) {
        Thread::SLEEP(200); return;
    }
    for (ST_INT32 i = 0; i < evCount; ++i) {
        std::pair<Iterator, Iterator> range = _mmap.equal_range (_events[i].data.fd);
        if (range.first == range.second) continue;

        struct sockaddr_in addr_in;
        socklen_t socklen = sizeof (struct sockaddr_in);
        ST_SOCKET clientfd = accept (_events[i].data.fd, (struct sockaddr*)&addr_in, &socklen);

        slogger()->LogDebug("tcpserver accept connected - %d - %s:%d", clientfd,
                        inet_ntoa (addr_in.sin_addr), ntohs (addr_in.sin_port));

        Iterator rangeit;
        for (rangeit = range.first; rangeit != range.second; ++rangeit) {
            if (!rangeit->second->hasRemoteAddr())
                continue;
            if ( rangeit->second->AddClient (clientfd, addr_in))
                break;
        }
        if (rangeit != range.second)
            continue;
        // 找到第一个接受任意IP的通道
        for (rangeit = range.first; rangeit != range.second; ++rangeit)
            if ( rangeit->second->AddClient (clientfd, addr_in))
                break;
        // 没通道关心它, 关掉它
        if (rangeit == range.second) {
            close (clientfd);
        }
    }
}

ST_VOID * TcpSocketSharer::acceptProc (void * param)
{
	TcpSocketSharer* ptr = (TcpSocketSharer*) param;
    if (! ptr->isThreadWork()) return 0;
    ptr->Accept();
}

PortTcpServer::PortTcpServer(Channel *channel, PortInfo *portInfo):
PortBase(channel, portInfo),
m_server_fd(-1),
m_epollfd(-1)
{
    Memset(&m_server_fd, 0, sizeof(m_server_fd));
}

PortTcpServer::~PortTcpServer()
{
    Stop();
    Uninit();
}

TcpSocketSharer& PortTcpServer::GetSharer()
{
    static TcpSocketSharer sharer;
    return sharer;
}

ST_VOID PortTcpServer::Init()
{
    if(m_Inited) return;
    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_port   = htons(m_pPortInfo->LocalPort);
    m_server_addr.sin_addr.s_addr = inet_addr(m_pPortInfo->LocalAddress);
    m_Inited = true;
}

ST_VOID PortTcpServer::Uninit()
{
    if(!m_Inited) return;
    this->Close();
    m_Inited = false;
}

ST_VOID PortTcpServer::Open()
{
    if(m_IsOpened) return;

    m_epollfd = epoll_create(MAXEPOLLSIZE);  //创建一个epoll的事例，通知内核需要监听size个fd

    this->GetSharer().Open(this);

    m_IsOpened = true;
}

ST_VOID PortTcpServer::Close()
{
    this->GetSharer().Close(this);

    m_IsOpened = false;
}

ST_VOID PortTcpServer::Close(ST_UINT64 port)
{
    struct in_addr addr;
    addr.s_addr = port;
    ClientItor itor = FindClient (addr);
    if (itor != m_clients.end ())
        CloseClient(itor->first);
}

ST_BOOLEAN PortTcpServer::IsOpened()
{
    return m_IsOpened;
}

ST_VOID PortTcpServer::Work()
{
    PortBase::Work();
    this->GetSharer().ThreadWork();
}

ST_VOID PortTcpServer::Stop()
{
    PortBase::Stop();
    this->GetSharer().ThreadStop();
}

ST_VOID PortTcpServer::Recv()
{
    if(!m_IsOpened) return;
    if (m_epollfd == -1) return;
    ST_INT32 eventSize = epoll_wait(m_epollfd, this->m_readEvents, MAXEPOLLSIZE, -1);
    if (eventSize == -1) {
        Thread::SLEEP(500);
        return;
    }
    for(int i = 0; i < eventSize; ++i) {
        if(m_readEvents[i].events & EPOLLIN) {
            if(ProcessRead(m_readEvents[i].data.fd) < 0)
            {
                ClientItor itor = m_clients.find (m_readEvents[i].data.fd);
                if(itor != m_clients.end()) {
                    m_pChannel->DisposeDataCache(itor->second.addr_in.sin_addr.s_addr);
                }
                CloseClient(m_readEvents[i].data.fd);
            }
        }
    }
}

ST_BOOLEAN PortTcpServer::Send(ST_UINT64 portAddr, ST_BYTE *buf, ST_UINT size)
{
    if(portAddr == -1) {
        for(ClientItor itor = m_clients.begin(); itor != m_clients.end(); itor++)
        {
            return ProcessWrite(itor->first, buf, size) == 0;
        }
    }
    else {
        struct in_addr addr;
        addr.s_addr = portAddr;
        ClientItor itor = FindClient (addr);
        if(itor != m_clients.end()) {
            if (ProcessWrite(itor->first, buf, size) == 0)
                return true;
            else {
                m_pChannel->DisposeDataCache(portAddr);
                CloseClient (itor->first);
            }
        }
        else{
            m_pLogger->LogDebug("%s Tcpserver When send data, can't find the client.",
                this->GetChannel()->GetChannelInfo()->ChannelName);
        }
    }
    return false;
}



bool PortTcpServer::hasRemoteAddr ()
{
    if (Strlen (this->GetPortInfo()->RemoteAddress) == 0)
        return false;
    if (INADDR_NONE == inet_addr(this->GetPortInfo()->RemoteAddress))
        return false;
    return true;
}

ST_INT PortTcpServer::ProcessRead(ST_SOCKET sock_fd)
{
    if(sock_fd <= 0) return 0;
    ST_INT readLen = recv(sock_fd, m_PortBuf, sizeof(m_PortBuf), 0);
    if(readLen > 0) {
        ClientItor itor = m_clients.find (sock_fd);
        if(itor == m_clients.end()) {
            return -1;
        }
        m_portTask.PortDstAddr = itor->second.addr_in.sin_addr.s_addr;
        m_portTask.Write(m_PortBuf, readLen);
        m_portTask.LocalChannelID = m_pChannel->GetLocalChannelID();
        m_portTask.KnowIPAddr = true;
        m_portTask.DeviceAddr = -1;
        m_pChannel->GetCEngine()->ReadTask(&m_portTask);
    }
    else if(readLen < 0 && (EAGAIN == errno || EINTR == errno)) {
        return 0;
    }
    else {
        m_pLogger->LogDebug("%s tcpserver recv error, errno: %d, desc: %s",
            this->GetChannel()->GetChannelInfo()->ChannelName, errno, strerror(errno));
        return -1;
    }
    return 0;
}

ST_INT PortTcpServer::ProcessWrite(ST_SOCKET sock_fd, ST_BYTE *buf, ST_UINT size)
{
    if(sock_fd <= 0) return 0;
    ST_INT nLeft = size, sendLen = 0;
    while(nLeft > 0) {
        if(sock_fd <= 0) break;
        sendLen = send(sock_fd, buf, nLeft, 0);
        if(sendLen > 0) {
            buf += sendLen;
            nLeft -= sendLen;
        }
        else if(sendLen < 0 && (errno == EAGAIN || errno == EINTR)) {
            Thread::SLEEP(10);
            continue;
        }
        else {
            m_pLogger->LogDebug("%s tcpserver send error, errno: %d, desc: %s",
                this->GetChannel()->GetChannelInfo()->ChannelName, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

PortTcpServer::ClientItor PortTcpServer::FindClient (const struct sockaddr_in& addrin)
{
    for (ClientItor it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (sockaddr_equal(it->second.addr_in, addrin))
            return it;
    }
    return m_clients.end();
}

PortTcpServer::ClientItor PortTcpServer::FindClient (const struct in_addr& addr)
{
    for (ClientItor it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second.addr_in.sin_addr.s_addr == addr.s_addr)
            return it;
    }
    return m_clients.end();
}

bool PortTcpServer::AddClient  (ST_SOCKET sockfd, struct sockaddr_in& addr_in)
{
    if (m_clients.size () + 1 > m_pChannel->GetChannelInfo()->MaxConnects) {
        ClientItor itor = FindClient (addr_in.sin_addr);
        if (itor != m_clients.end()) {
            CloseClient (itor->first);
        }
        else
            return false;
    }

    in_addr_t value = inet_addr(this->GetPortInfo()->RemoteAddress);
    if (this->hasRemoteAddr() && addr_in.sin_addr.s_addr != value)
        return false;

    if(SetNonBlocking(sockfd) < 0)
        return false;
    SetKeepalive (sockfd, true, 300, 30, 3);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, sockfd, &event) < 0)
        return false;
    SockInfo info;
    info.socketfd = sockfd;
    info.addr_in.sin_addr.s_addr = addr_in.sin_addr.s_addr;
    info.addr_in.sin_port        = addr_in.sin_port;
    info.addr_in.sin_family      = addr_in.sin_family;
    {
        Locker locker(&(this->m_Mutex));
        m_clients.insert(make_pair(sockfd, info));
    }
    m_pChannel->GetCEngine()->OnConnect(sockfd, addr_in.sin_addr.s_addr);
    m_pLogger->LogDebug("tcpserver add client fd: %d", sockfd);
    return true;
}

ST_VOID PortTcpServer::CloseClient (ST_SOCKET sockfd, bool remove)
{
    ClientItor itor = m_clients.find (sockfd);
    if (itor == m_clients.end()) {
        return;
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, &event);
    shutdown(sockfd, 2);
    close(sockfd);
    m_pLogger->LogDebug("%s tcpserver close client fd: %d.",
        this->GetChannel()->GetChannelInfo()->ChannelName, sockfd);
    if (remove) {
            Locker locker(&(this->m_Mutex));
            m_clients.erase(itor);
    }
    m_pChannel->GetCEngine()->OnDisconnect(sockfd, itor->second.addr_in.sin_addr.s_addr);
}

ST_VOID PortTcpServer::CloseAllClient()
{
    ClientItor itor = m_clients.begin();
    for (; itor != m_clients.end(); ++itor) {
        m_pChannel->DisposeDataCache(itor->second.addr_in.sin_addr.s_addr);
        CloseClient (itor->first, false);
    }
    m_clients.clear();
}

