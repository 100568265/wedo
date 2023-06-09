//#include "stdafx.h"
#include "PortTcpClient.h"
#include "Channel.h"
#include "sysmutex.h"
#include "PortTcp.h"

#include <sys/socket.h>
#include <sys/reboot.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#include "wedo/net/socket.h"

#define _ENABLE_GETADDRINFO 1

static int parse_address (const char * hostname, uint16_t port,
                            struct sockaddr_storage & addr)
{
    if (!hostname)
        return -1;

    memset (&addr, 0, sizeof(addr));
{
    struct in_addr tmpaddr;
    if (inet_pton(AF_INET, hostname, &tmpaddr) == 1) {
        // Is a specific IPv4 address, e.g. 192.168.1.66:2333
        struct sockaddr_in * psa = (struct sockaddr_in*)&addr;

        psa->sin_family      = AF_INET;
        psa->sin_port        = htons(port);
        psa->sin_addr        = tmpaddr;
        return 0;
    }
}
{
    struct in6_addr tmpaddr;
    if (inet_pton(AF_INET6, hostname, &tmpaddr) == 1) {
        // IPv6 address, e.g. [3ffe:2a00:100:7031::1]:2333
        struct sockaddr_in6 * psa = (struct sockaddr_in6*)&addr;

        psa->sin6_family    = AF_INET6;
        psa->sin6_port      = htons(port);
        psa->sin6_addr      = tmpaddr;
        return 0;
    }
}
#if _ENABLE_GETADDRINFO
    char port_str[8] = {0};
    snprintf(port_str, sizeof(port_str), "%u", port);

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_protocol   = IPPROTO_TCP;

    if (getaddrinfo(hostname, port_str, &hints, &servinfo) != 0) {
        return -1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        memcpy(&addr, p->ai_addr, p->ai_addrlen);
    }
    freeaddrinfo(servinfo);
    return 0;
#else
    struct hostent * he = gethostbyname(hostname);
    if (he == NULL) {
        return -1;
    } else {
        struct sockaddr_in * psa  = (struct sockaddr_in *)&addr;
        struct sockaddr_in6* psa6 = (struct sockaddr_in6*)&addr;

        void * pa = (he->h_addrtype == AF_INET ?
                        (void*)&psa->sin_addr : (void*)&psa6->sin6_addr);
        memcpy(pa, he->h_addr_list[0], he->h_length);

        psa->sin_family      = he->h_addrtype;
        psa->sin_port        = htons((uint16_t) port);
    }
    return 0;
#endif /* _ENABLE_GETADDRINFO */
}

#if defined(_WIN32)
    typedef char * buffer_type;
#else
    typedef void * buffer_type;
#endif

PortTcpClient::PortTcpClient(Channel * channel, PortInfo * portInfo):
PortBase(channel, portInfo),
_state(NoConnect)
{
    isPWork = false;
    firstConnect = false;
    oldcurtime = clock();
    newcurtime = clock();
}

PortTcpClient::~PortTcpClient()
{
	Stop();
    Uninit();
}

ST_VOID PortTcpClient::Init()
{
    if (m_Inited) return ;

    memset (&_dest_addr, 0, sizeof(_dest_addr));

/*
    Memset(&m_Server_addr, 0, sizeof(m_Server_addr));
    m_Server_addr.sin_family      = AF_INET;
    m_Server_addr.sin_port        = htons(m_pPortInfo->RemotePort);
    m_Server_addr.sin_addr.s_addr = inet_addr(m_pPortInfo->RemoteAddress);

    memcpy (&_dest_addr, &m_Server_addr, sizeof(m_Server_addr));
*/
    m_Inited = true;
}

ST_VOID PortTcpClient::Uninit()
{
    if(!m_Inited) return ;

    this->Close();

    pingThread.Stop();
    isPWork = false;
    m_Inited = false;
}

ST_VOID PortTcpClient::Open()
{

    {
        Locker locker(&(this->m_Mutex));
 //       if(State() != NoConnect)//linweiming
  //          return;
   //     SetState(Connecting);
    }
    Strcpy(m_RemoteAddress,m_pPortInfo->RemoteAddress);
    if (parse_address (m_pPortInfo->RemoteAddress, m_pPortInfo->RemotePort, _dest_addr))
    {
        Thread::SLEEP (500);
        SetState(NoConnect);
        return;
    }

    socket_ptr sptr(new wedo::socket(::socket(_dest_addr.ss_family, SOCK_STREAM, IPPROTO_TCP)));
	if(sptr->fd() == -1)
    {
        int err_ = errno;
        // err_handler
		return;

    }

    wedo::socket::set_cloexec   (*sptr);
//    SetLinger   (sptr.fd(), 0, 0);
    SetKeepalive(sptr->fd(), 1, 180, 60, 20);

    if(connect(sptr->fd(), (struct sockaddr *)&_dest_addr, sizeof(_dest_addr)) == -1)
    {
        int err_ = errno;
        switch (err_) {
            case EINPROGRESS:  case EINTR:       case EISCONN:
                break;

            case EAGAIN:       case EADDRINUSE:  case EADDRNOTAVAIL:
            case ECONNREFUSED: case ENETUNREACH:
                Thread::SLEEP (500);
                SetState(NoConnect);
                return;

            case EACCES:       case EPERM:       case EAFNOSUPPORT:
            case EALREADY:     case EBADF:       case EFAULT:
            case ENOTSOCK:
                m_pLogger->LogWarn("tcpclient connect error: %d, desc: %s", err_, strerror(err_));
                // no break;
            default:
                // logger << "connect error, errno:" << err_;
                Thread::SLEEP (500);
                SetState(NoConnect);
                return;
        }
    }
    {
        Locker locker(&(this->m_Mutex));
        _socket = sptr;
        SetState(Connected);
        m_IsOpened = true;
    }
    // m_Mutex.Notify();
	m_pChannel->GetCEngine()->OnConnect(_socket->fd(), 0);
    m_pLogger->LogDebug("tcpclient connected to %s.", m_pPortInfo->RemoteAddress);
/*    if(!isPWork)                        //LiuXiaoBin Add in 2019-11-23
    {
        strcpy(gprsOnFileName,"gprs-on.sh");
        strcpy(dialOnFileName,"dial-on.sh");
        readProgramList();                       //默认文件名，如果有4g后缀则默认优先
        m_pLogger->LogDebug("start ping Thread");
        firstConnect = true;
        pingThread.Start(pingTaskProc,this,true);
        isPWork = true;
    }*/
}

ST_VOID PortTcpClient::Close()
{
    bool is_exist = false;
    wedo::socket::type fd_ = -1;
    {
        Locker locker(&(this->m_Mutex));
        if(State() == Connected)
        {
            SetState(Disconnecting);
            is_exist = true;
            fd_ = _socket->fd();
            shutdown(_socket->fd(), SHUT_WR);
            _socket.reset();
            SetState(NoConnect);
            m_IsOpened = false;
        }
    }
    if (is_exist) {
        m_pLogger->LogDebug("tcpclient close connect.");
        m_pChannel->GetCEngine()->OnDisconnect(fd_, 0);
    }
}


ST_BOOLEAN PortTcpClient::Send(ST_UINT64 portAddr, ST_BYTE *buf, ST_UINT size)
{
    if (State() != Connected) {
        Thread::SLEEP (100);
        return false;
    }
    socket_ptr sptr;
    {
        Locker locker(&(this->m_Mutex));
        if (State() != Connected)
            return false;                     // There should be no delay in the lock.
        sptr = _socket;
    }
    ST_INT lastlen = size, sendlen = 0;
    while (lastlen > 0) {
        sendlen = send(sptr->fd(), (buffer_type)buf, lastlen, 0);
        if(sendlen >= 0) {
            buf     += sendlen;
            lastlen -= sendlen;
            continue;
        }

        int err_ = errno;
        switch (err_) {
            case EAGAIN:        case EINTR://         case EWOULDBLOCK:
                Thread::SLEEP(100);
                continue;
            case EMSGSIZE:      case ENOBUFS:
                return false;

//            case EPIPE:         case ECONNRESET:
            default:
                this->Close();
            // logger  ...;
                return false;
        }
    }
    return true;
}

ST_VOID PortTcpClient::Recv()
{
    if (State() != Connected) {
        Thread::SLEEP (1000);
        return;
    }
    socket_ptr sptr;
    {
        Locker locker(&(this->m_Mutex));
        if (State() != Connected)
            return;                     // There should be no delay in the lock.
        sptr = _socket;
    }
    FD_ZERO  (&m_ReadSet);             //清空合集
    FD_SET   (sptr->fd(), &m_ReadSet);

    struct timeval iv = { 3, 0 };
    int ret = select(sptr->fd() + 1, &m_ReadSet, NULL, NULL, &iv);
    if(!ret) return;

    if (ret < 0)
    {
        m_pLogger->LogDebug("tcpclient select error: %d,desc: %s", errno, strerror(errno));
        return;
    }

    if(! FD_ISSET(sptr->fd(), &m_ReadSet))
        return;

    int readlen = recv(sptr->fd(), (buffer_type)m_PortBuf, 1024, 0);
    if (readlen > 0) {
        m_portTask.Write(m_PortBuf, readlen);
        m_portTask.PortDstAddr = 0; //inet_addr(m_pPortInfo->RemoteAddress);
        m_portTask.LocalChannelID = m_pChannel->GetLocalChannelID();
        m_pChannel->GetCEngine()->ReadTask(&m_portTask);
    }
    else if(readlen < 0 && (EAGAIN == errno || EINTR == errno || EWOULDBLOCK == errno))
    {
        return;
    }
    else {
        this->Close();
#ifndef _WIN32
        if (readlen)
            m_pLogger->LogDebug("tcpclient recv error: %d,desc: %s", errno, strerror(errno));
#endif
    }
}

/*
EAGAIN          11   Try again                    请重试
EINTR           4    Interrupted system call      系统调用被中断

*/

ST_VOID *  PortTcpClient::pingTaskProc(ST_VOID *param)
{
    //m_pPortInfo->RemoteAddress;
    Thread::SLEEP (6000);//6秒ping一次
    PortTcpClient *ptc = (PortTcpClient*)param;
    ptc->checkPing();
    return 0;
}

ST_VOID    PortTcpClient::checkPing()
{
    if((ping_status(m_RemoteAddress))==-1)
    {     //error
        if(firstConnect)
            firstConnect=false;
        lostNum += 1;
        m_pLogger->LogDebug("Ping %s error",m_RemoteAddress);
        oldcurtime = clock();
        if ((abs(oldcurtime - newcurtime) > 60*60* CLOCKS_PER_SEC)) //大于3600秒就是30分钟 断线一小时则重启系统设备
        {
            m_pLogger->LogDebug("Ping %s error more than 1 hour,reboot decvies.",m_RemoteAddress);
            sync(); // 同步磁盘数据,将缓存数据回写到硬盘,以防数据丢失[luther.gliethttp]
            reboot(RB_AUTOBOOT);
        }
    }
    else   //successful
    {
        newcurtime = clock();
        if(!firstConnect)
        {
            this->Close();
            firstConnect = true;
        }

        if(lostNum!=0)
            lostNum = 0;
    }
    if((lostNum)>=20)
    {
        m_pLogger->LogDebug("Ping lost More than 20 times,begin Restart 4G module ");
        Restart4Gmodule();
        lostNum = 0;
    }

}

ST_VOID *   PortTcpClient::RUNDialSript(ST_VOID *param)
{
    //"~/program/dial-on.sh"
    PortTcpClient *ptc = (PortTcpClient*)param;
    char dialon[128];
    sprintf(dialon,"~/program/%s",ptc->dialOnFileName);
    system(dialon);
    return 0;
}

ST_VOID   PortTcpClient::Restart4Gmodule()
{
    //"~/program/gprs-on.sh"
    char gprson[128];
    sprintf(gprson,"~/program/%s",gprsOnFileName);

    this->Close();
    if(RunShellScript("~/program/gprs-off.sh"))
    {
        Thread::SLEEP (1000);
        if(RunShellScript(gprson))
        {
            Thread::SLEEP (1000);
            m_4gThread.Start(RUNDialSript, this, false);

        }
        else
            m_pLogger->LogWarn("Run gprs-on error");
    }
    else
       m_pLogger->LogWarn("Run gprs-off error");
    return;
}
ST_BOOLEAN   PortTcpClient::RunShellScript(const ST_CHAR *fileName)
{
    FILE *fp;
    char buffer[80];
    fp=popen(fileName,"r");
    fgets(buffer,sizeof(buffer),fp);
    if(fp==NULL)
        return false;
    m_pLogger->LogDebug("Run ShellScript : %s",buffer);
    return true;
}

int PortTcpClient::ping_status(char *ip)
{
    int i, status;
    pid_t pid;
    printf(">>>>>>>>>>>>> ping_status\n ");
    // 不同则循环检测多次
    for (i = 0; i < 3; ++i)
    {
        // 新建一个进程来执行ping命令
        if ((pid = vfork()) < 0)
        {
            //printf("vfork error");
            m_pLogger->LogDebug("ping vfork error");
            continue;
        }

        if (pid == 0)
        {
            if ( execlp("ping", "ping","-c","1",ip, (char*)0) < 0)
            {
                printf("execlp error\n");
                m_pLogger->LogDebug("ping execl error");
                //exit(1);
                return -1;
            }
        }

        waitpid(pid, &status, 0);

        // 相等说明正常
        if (status == 0)
            return 0;

      //  sleep(2);
    }

    return -1;
}


ST_VOID     PortTcpClient::readProgramList()
{
    DIR *dir;
    struct dirent *ptr;
    char basePath[32] ;
    Strcpy(basePath,"../../program/");
//    ="../../program/";

    if ((dir=opendir(basePath)) == NULL)
    {
        m_pLogger->LogWarn("Load program file error");
        return ;
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            if(strcmp(ptr->d_name,"gprs-on-4g.sh")==0)
                strcpy(gprsOnFileName,ptr->d_name);
            else if((strcmp(ptr->d_name,"dial-on-4g.sh")==0))
                strcpy(dialOnFileName,ptr->d_name);
        }
    }
    closedir(dir);
}
