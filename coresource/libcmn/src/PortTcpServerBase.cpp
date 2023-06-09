//#include "stdafx.h"
#include "PortTcpServerBase.h"
#include "Channel.h"

PortTcpServerBase::PortTcpServerBase(Channel *channel, PortInfo *portInfo):PortBase(channel, portInfo)
{
    m_SockLen= sizeof(struct sockaddr_in);
    m_Server_listen_fd=-1;
    m_Client_connected_fd=-1;
}

PortTcpServerBase::~PortTcpServerBase()
{
	Stop();
	Uninit();
}

ST_VOID PortTcpServerBase::Init()
{
    if(m_Inited) return ;
    Memset(&m_Server_addr,0,sizeof(m_Server_addr));
    m_Server_addr.sin_family=AF_INET;
    m_Server_addr.sin_port = htons(m_pPortInfo->LocalPort);
    m_Server_addr.sin_addr.s_addr =inet_addr(m_pPortInfo->LocalAddress); //htonl(INADDR_ANY)
    m_Inited=true;
}

ST_VOID PortTcpServerBase::Uninit()
{
    if(!m_Inited) return;
    Close();
    m_Inited=false;
}

ST_VOID PortTcpServerBase::Open()
{
	if(m_IsOpened) return;
	Locker locker(&(this->m_Mutex));

#ifdef _WIN32
	if(INVALID_SOCKET != m_Server_listen_fd) {
		closesocket(m_Server_listen_fd);
	}
	if(INVALID_SOCKET == (m_Server_listen_fd = socket(AF_INET,SOCK_STREAM,0)))
		return;
#else
    if(-1 == (m_Server_listen_fd = socket(AF_INET, SOCK_STREAM, 0)))
        return;

	SetNonBlocking(m_Server_listen_fd);
#endif
    SetReuseAddr(m_Server_listen_fd, 1);
    SetLinger(m_Server_listen_fd, 0, 0);
    SetKeepalive(m_Server_listen_fd, 1, 300, 30, 3);
    if(-1 == Listen()) {
        return;
	}
#ifdef _WIN32
	FD_ZERO(&m_ReadSet);
	FD_SET(m_Server_listen_fd, &m_ReadSet);
#elif __linux__
    m_epoll_read_fd = epoll_create(MAX_EPOLLSIZE);
    ev.events = EPOLLIN|EPOLLET;
    ev.data.fd = m_Server_listen_fd;
    if (epoll_ctl(m_epoll_read_fd, EPOLL_CTL_ADD, m_Server_listen_fd, &ev) < 0)  return;
#elif __unix__
    FD_ZERO(&m_ReadSet);
    m_ClientThread.Start(AcceptThread, this, true);
#endif
    m_IsOpened = true;
}

ST_VOID PortTcpServerBase::Stop()
{
#ifdef _WIN32
	if(!m_Working) return;
	closesocket(m_Server_listen_fd);
	closesocket(m_Client_connected_fd);
	m_portThread.Stop(10);
	m_Working=false;
#endif
}

ST_VOID PortTcpServerBase::Close()
{
    m_IsOpened = false;
#ifdef _WIN32
    if(INVALID_SOCKET != m_Server_listen_fd){
        closesocket(m_Client_connected_fd);
        closesocket(m_Server_listen_fd);
        WSACleanup();
    }
#elif __linux__
    if(-1 != m_Server_listen_fd){
        close(m_Client_connected_fd);
        close(m_Server_listen_fd);
    }
#elif __unix__
    m_ClientThread.Stop(10);
#endif
}

ST_VOID PortTcpServerBase::CloseClient(ST_SOCKET &sock_fd)
{
	Locker locker(&(this->m_Mutex));

#ifdef _WIN32
	shutdown(sock_fd, 2);
    closesocket(sock_fd);
	FD_CLR(sock_fd, &m_ReadSet);
#elif __linux__
    ev.events = EPOLLIN;
    ev.data.fd = sock_fd;
    epoll_ctl(m_epoll_read_fd, EPOLL_CTL_DEL, sock_fd, &ev);
	shutdown(sock_fd, 2);
    close(sock_fd);
#elif __unix__
    FD_CLR(sock_fd, &m_ReadSet);
#endif
	sock_fd = -1;
}

ST_BOOLEAN PortTcpServerBase::IsOpened()
{
    return m_IsOpened;
}

ST_VOID PortTcpServerBase::Recv()
{
	if(!m_IsOpened) return;
#ifdef _WIN32
	fd_set tmpSet = m_ReadSet;
	if(select(2, &tmpSet, NULL, NULL, NULL) > 0) {
		if(FD_ISSET(m_Server_listen_fd,&tmpSet)) {
			Accept();
		}
		else{
			if(FD_ISSET(m_Client_connected_fd,&tmpSet)){
				if(ProcessRead(m_Client_connected_fd)<0){
					CloseClient(m_Client_connected_fd);
				}
			}
		}
		FD_ZERO(&tmpSet);
	}
#elif __linux__
    m_REvents = epoll_wait(m_epoll_read_fd, m_ReadEvents, MAX_EPOLLSIZE , -1);
    if (m_REvents == -1) {
        Thread::SLEEP(1000);
        return;
    }
    for(int i = 0; i < m_REvents; ++i ){
        if( m_ReadEvents[i].data.fd == m_Server_listen_fd ){
            if(Accept()<0) continue;
        }
    }
#endif
}



ST_BOOLEAN PortTcpServerBase::Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size)
{
    if(ProcessWrite(m_Client_connected_fd, buf, size) == 0) {
        return true;
    }
    else{
        CloseClient(m_Client_connected_fd);
        return false;
    }
}

ST_INT PortTcpServerBase::ProcessRead(ST_SOCKET sock_fd)
{
#ifdef _WIN32
	if(sock_fd == INVALID_SOCKET) return 0;
	ST_INT readLen = recv(sock_fd,(char*)m_PortBuf,1024,0);
#else
	if(sock_fd<=0) return 0;
    ST_INT readLen = recv(sock_fd,m_PortBuf,1024,0);
#endif
    if(readLen>0){
		Thread::SLEEP(100);
    }
#ifdef _WIN32
	else if(readLen<0 && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError()==WSAEINTR )){
		return 0;
    }
#else
    else if(readLen<0 && (EAGAIN==errno || EINTR==errno)){
        return 0;
    }
#endif
    else{
        return -1;
    }
    return 0;
}

ST_INT PortTcpServerBase::ProcessWrite(ST_SOCKET sock_fd,ST_BYTE *buf,ST_UINT size)
{
#ifdef _WIN32
	if(sock_fd == INVALID_SOCKET) return 0;
#else
	if(sock_fd <= 0) return 0;
#endif
    ST_INT nLeft = size, sendLen = 0;
    while(nLeft > 0) {
#ifdef _WIN32
		sendLen = send(sock_fd, (char*)buf, nLeft, 0);
#else
    if(sock_fd <= 0) break;
    try {
        sendLen = send(sock_fd, buf, nLeft, 0);
        Thread::SLEEP(20);
	}
	catch(...) {
		;
	}
#endif
        if(sendLen > 0) {
            buf   += sendLen;
            nLeft -= sendLen;
        }
/*#ifdef _WIN32
		else if(sendLen<0 && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError()==WSAEINTR )){
			Thread::SLEEP(10);
			continue;
        }
#else
        else if(sendLen<0 && (errno==EAGAIN || errno==EINTR) ) {
            Thread::SLEEP(10);
            continue;
        }
#endif*/
        else {
			#ifndef _WIN32
			#endif
            return -1;
        }
    }
    return 0;
}

ST_INT PortTcpServerBase::Listen()
{
    if (0 > bind(m_Server_listen_fd, (struct sockaddr*)&m_Server_addr, sizeof(m_Server_addr))){
        Close();
        return -1;
    }
    if (0 > listen(m_Server_listen_fd, 1/*linweiming MAXLISTENQUE*/)){
		Close();
		return -1;
    }
    return 0;
}

ST_INT PortTcpServerBase::Accept()
{
	CloseClient(m_Client_connected_fd);
    m_Client_connected_fd = accept(m_Server_listen_fd, (struct sockaddr*)&m_Client_addr,&m_SockLen);
#ifdef _WIN32
	if(m_Client_connected_fd==INVALID_SOCKET) return -1;
#else
	if (m_Client_connected_fd < 0) return -1;
#endif
    if(SetNonBlocking(m_Client_connected_fd) < 0 ) return -1;
#ifdef _WIN32
	FD_SET(m_Client_connected_fd,&m_ReadSet);
#elif __linux__
    ev.events = EPOLLIN;
    ev.data.fd = m_Client_connected_fd;
    if(epoll_ctl(m_epoll_read_fd, EPOLL_CTL_ADD, m_Client_connected_fd, &ev ) < 0 ) return -1;
#else
    FD_SET(m_Client_connected_fd,&m_ReadSet);
#endif
    return 0;
}


#ifdef _WIN32
ST_UINT32 __stdcall PortTcpServerBase::AcceptThread(ST_VOID *param)
{
	PortTcpServerBase *pServer=(PortTcpServerBase*)param;
	pServer->Accept();
    return 0;
}
#elif __unix__
ST_VOID *PortTcpServerBase::AcceptThread(ST_VOID *param)
{
    PortTcpServerBase *pServer=(PortTcpServerBase*)param;
    pServer->Accept();
    return 0;
}
#endif
