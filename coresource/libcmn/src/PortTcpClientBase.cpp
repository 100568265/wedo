//#include "stdafx.h"
#include "PortTcpClientBase.h"
#include "Channel.h"
#include "MonitorClient.h"


PortTcpClientBase::PortTcpClientBase(Channel *channel,PortInfo *portInfo):PortBase(channel,portInfo)
{
    client_socket_fd=-1;
}

PortTcpClientBase::~PortTcpClientBase()
{
	Stop();
    Uninit();
}

ST_VOID PortTcpClientBase::Init()
{
    //if(m_Inited) return ;
    Memset(&m_Server_addr,0,sizeof(m_Server_addr));
    m_Server_addr.sin_family=AF_INET;
    m_Server_addr.sin_port = htons(m_pPortInfo->RemotePort);
    m_Server_addr.sin_addr.s_addr =inet_addr(m_pPortInfo->RemoteAddress);
    //m_Inited=true;
}

ST_VOID PortTcpClientBase::Uninit()
{
    if(!m_Inited) return ;
    Close();
    m_Inited=false;
}

ST_VOID PortTcpClientBase::Open()
{
	if(m_IsOpened) return;
	m_Mutex.Lock();
	if(-1!=client_socket_fd){
#ifdef _WIN32
		closesocket(client_socket_fd);
#else
		close(client_socket_fd);
#endif
	}
	if((client_socket_fd=socket(AF_INET,SOCK_STREAM,0))==-1) {
		goto exit;
	}
 //linweiming   SetKeepalive(client_socket_fd,1,30,5,3);
    if(connect(client_socket_fd,(struct sockaddr  *)&m_Server_addr,sizeof(struct sockaddr_in))==0){
        m_IsOpened=true;
        m_Mutex.Notify();
    }
exit:
	m_Mutex.UnLock();
}

ST_VOID PortTcpClientBase::Close()
{
	if(-1!=client_socket_fd){
#ifdef _WIN32
		closesocket(client_socket_fd);
#else
		close(client_socket_fd);
#endif
	}
    m_IsOpened=false;
}



ST_BOOLEAN PortTcpClientBase::Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size)
{
    ST_INT nLeft=size,sendLen=0;
//    if(!IsOpened()) Open();
    if(IsOpened()){
        while(nLeft>0){
#ifdef _WIN32
			sendLen=send(client_socket_fd,(char*)buf,nLeft,0);
#else
            sendLen=send(client_socket_fd,buf,nLeft,0);
#endif
            if(sendLen>0){
                buf+=sendLen;
                nLeft-=sendLen;
            }
#ifdef _WIN32
			else if(sendLen<0 && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError()==WSAEINTR )){
				Thread::SLEEP(100);
				continue;
			}
#else
            else if(sendLen<0 && (errno==EAGAIN || errno==EINTR) ) {
                Thread::SLEEP(100);
                continue;
            }
#endif
            else{
                Close();
                return false;
            }
        }
        return true;
    }
    else{
        return false;
    }
}

ST_VOID PortTcpClientBase::Recv()
{
    while(!IsOpened()){
        m_Mutex.Wait(-1);
    }
    FD_ZERO(&m_ReadSet);
    FD_SET(client_socket_fd,&m_ReadSet);
    if(select(client_socket_fd+1,&m_ReadSet,NULL,NULL,NULL)>0){
        if(FD_ISSET(client_socket_fd, &m_ReadSet)){
#ifdef _WIN32
			ST_INT readLen=recv(client_socket_fd,(char*)m_PortBuf,1024,0);
#else
            ST_INT readLen=recv(client_socket_fd,m_PortBuf,1024,0);
#endif
            if(readLen>0){
                m_portTask.Write(m_PortBuf,readLen);
				m_client->ReadTask(m_portTask);
                return;
            }
#ifdef _WIN32
			else if(readLen<0 && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError()==WSAEINTR )){
				return;
            }
#else
            else if(readLen<0 && (EAGAIN==errno || EINTR==errno)){
                return;
            }
#endif
        }
        else return;
    }
    Close();
}
