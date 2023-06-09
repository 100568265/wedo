//#include "stdafx.h"
#include "PortUdp.h"
#include "Channel.h"

PortUdp::PortUdp(Channel *channel,PortInfo *portInfo):PortBase(channel,portInfo)
{
    m_Udp_socket_fd=-1;
}

PortUdp::~PortUdp()
{
	Stop();
    Uninit();
}

ST_VOID PortUdp::Init()
{
    if(m_Inited) return;
    Memset(&m_Local_addr,0,sizeof(m_Local_addr));
    Memset(&m_Remote_addr,0,sizeof(m_Remote_addr));
    m_Local_addr.sin_family=AF_INET;
    m_Local_addr.sin_port=htons(m_pPortInfo->LocalPort);
    if(Strcmp(m_pPortInfo->LocalAddress,"")==0){
        m_Local_addr.sin_addr.s_addr=INADDR_ANY;
    }
    else{
        m_Local_addr.sin_addr.s_addr=inet_addr(m_pPortInfo->LocalAddress);
    }
    m_Addr_local_len=sizeof(m_Local_addr);
    m_Remote_addr.sin_family=AF_INET;
    m_Remote_addr.sin_port=htons(m_pPortInfo->RemotePort);
    m_Remote_addr.sin_addr.s_addr=inet_addr(m_pPortInfo->RemoteAddress);
    m_Addr_remote_len=sizeof(m_Remote_addr);
    m_Inited=true;
}

ST_VOID PortUdp::Uninit()
{
    if(!m_Inited) return;
    Close();
    m_Inited=false;
}

ST_VOID PortUdp::Open()
{
	if(m_IsOpened) return;
	m_Mutex.Lock();
	if(-1!=m_Udp_socket_fd){
#ifdef _WIN32
		closesocket(m_Udp_socket_fd);
#else
		close(m_Udp_socket_fd);
#endif
	}
	if((m_Udp_socket_fd=socket(AF_INET,SOCK_DGRAM,0))==-1){
		goto exit ;
	}
    SetNonBlocking(m_Udp_socket_fd);
	if(bind(m_Udp_socket_fd,(struct sockaddr*)&m_Local_addr,sizeof(m_Local_addr))<0){
		goto exit;
	}
    if(m_pPortInfo->Multicast!=0){
		if(AddMultiGroup(m_Udp_socket_fd,m_pPortInfo->RemoteAddress)<0) {
			goto exit;
		}
    }
    m_Mutex.Notify();
    m_IsOpened=true;
	m_pChannel->GetCEngine()->OnConnect(m_Udp_socket_fd,m_Udp_socket_fd);
exit:
	m_Mutex.UnLock();
}

ST_VOID PortUdp::Close()
{
    if(-1!=m_Udp_socket_fd){
#ifdef _WIN32
		closesocket(m_Udp_socket_fd);
#else
        close(m_Udp_socket_fd);
#endif
    }
	m_pChannel->GetCEngine()->OnDisconnect(m_Udp_socket_fd,m_Udp_socket_fd);
}

ST_BOOLEAN PortUdp::IsOpened()
{
    return m_IsOpened;
}

ST_VOID PortUdp::Recv()
{
    while(!IsOpened()){
        m_Mutex.Wait(-1);
    }
    FD_ZERO(&m_ReadSet);
    FD_SET(m_Udp_socket_fd,&m_ReadSet);
    if(select(m_Udp_socket_fd+1,&m_ReadSet,NULL,NULL,NULL)>0){
        if(FD_ISSET(m_Udp_socket_fd, &m_ReadSet)){
#ifdef _WIN32
			ST_INT readLen=recvfrom(m_Udp_socket_fd,(char*)m_PortBuf,1024,0, (struct sockaddr *)&m_Remote_addr, &m_Addr_remote_len);
#else
            ST_INT readLen=recvfrom(m_Udp_socket_fd,m_PortBuf,1024,0, (struct sockaddr *)&m_Remote_addr, &m_Addr_remote_len);
#endif
            if(readLen>0){
                m_portTask.Write(m_PortBuf,readLen);
                m_portTask.PortDstAddr=-1;//inet_addr(m_pPortInfo->RemoteAddress);
                m_portTask.LocalChannelID=m_pChannel->GetLocalChannelID();
                m_pChannel->GetCEngine()->ReadTask(&m_portTask);
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
            else{
				#ifndef _WIN32
                m_pLogger->LogDebug("udp recvfrom error,errno=%d,desc=%s",errno,strerror(errno));
				#endif
            }
        }
        else{
            return;
        }
    }
    else{
        Close();
        m_pLogger->LogDebug("upd closed.");
    }
}



ST_BOOLEAN PortUdp::Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size)
{
    ST_INT nLeft=size,sendLen=0;
    if(IsOpened()){
        while(nLeft>0){
#ifdef _WIN32
			sendLen=sendto(m_Udp_socket_fd,(char*)buf,nLeft,0,(struct sockaddr *)&m_Remote_addr, m_Addr_remote_len);
#else
            sendLen=sendto(m_Udp_socket_fd,buf,nLeft,0,(struct sockaddr *)&m_Remote_addr, m_Addr_remote_len);
#endif
            if(sendLen>0){
                buf+=sendLen;
                nLeft-=sendLen;
            }
            else{
				#ifndef _WIN32
                m_pLogger->LogDebug("udp sendto error,errno=%d,desc=%s",errno,strerror(errno));
				#endif
                return false;
            }
        }
        return true;
    }
    else{
        m_pLogger->LogDebug("udp sendto error,m_IsOpened=false.");
        return false;
    }
}
