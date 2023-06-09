//#include "stdafx.h"
#include "PortTcp.h"

#if defined(_WIN32)
    WSADATA wsaGlobalData;
    typedef char * optval_type;
#else
    typedef void * optval_type;
#endif


PortTcp::PortTcp()
{
#ifdef _WIN32
	WSAStartup(MAKEWORD(2, 2), &wsaGlobalData);		//版本约定
#endif
}

PortTcp::~PortTcp()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

ST_INT PortTcp::SetNonBlocking(ST_SOCKET socket_fd) ///< 设置sockfd为非阻塞模式
{
#ifdef _WIN32
	unsigned long is_enalbe = 1;
	return (ioctlsocket(socket_fd, FIONBIO, &is_enalbe) ? -1: 0);
#else
    int flag = fcntl(socket_fd, F_GETFL, 0);
    if (flag == -1) return -1;
    return fcntl(socket_fd, F_SETFL, flag | O_NONBLOCK);
#endif
}
/*
使用说明：

int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);

int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);

参数：
sock：将要被设置或者获取选项的套接字。
level：选项所在的协议层。
optname：需要访问的选项名。
optval：对于getsockopt()，指向返回选项值的缓冲。对于setsockopt()，指向包含新选项值的缓冲。
optlen：对于getsockopt()，作为入口参数时，选项值的最大长度。作为出口参数时，选项值的实际长度。对于setsockopt()，现选项的长度。

返回说明：
成功执行时，返回0。失败返回-1，errno被设为以下的某个值
EBADF：sock不是有效的文件描述词
EFAULT：optval指向的内存并非有效的进程空间
EINVAL：在调用setsockopt()时，optlen无效
ENOPROTOOPT：指定的协议层不能识别选项
ENOTSOCK：sock描述的不是套接字

*/
ST_VOID PortTcp::SetKeepalive(ST_SOCKET socket_fd, ST_INT kpalive,
                              ST_INT kpidle, ST_INT kpintval, ST_INT kpcout)
{
#ifdef  _WIN32
	DWORD dw;
	tcp_keepalive alive_in  = {0};
	tcp_keepalive alive_out = {0};
	alive_in.keepalivetime  = kpidle;                // 开始首次KeepAlive探测前的TCP空闭时间
	alive_in.keepaliveinterval = kpintval;           // 两次KeepAlive探测间的时间间隔
	alive_in.onoff  = TRUE;
	setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&kpalive , sizeof(kpalive));
    WSAIoctl(socket_fd,SIO_KEEPALIVE_VALS,&alive_in,sizeof(alive_in),&alive_out,sizeof(alive_out), &dw,NULL,NULL);
#else
	ST_INT keepalive = kpalive;//1;
    ST_INT keepidle = kpidle;//30;
    ST_INT keepinterval = kpintval;//5;
    ST_INT keepcount =kpcout;// 3;
    setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE,  (void*)&keepalive ,    sizeof(keepalive));     // 开启keepalive属性
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPIDLE,  (void*)&keepidle ,     sizeof(keepidle));      // 如该连接在60秒内没有任何数据往来,则进行探测
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPINTVL, (void*)&keepinterval , sizeof(keepinterval));  // 探测时发包的时间间隔为5 秒
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPCNT,   (void*)&keepcount ,    sizeof(keepcount));     // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
#endif
}

ST_INT PortTcp::SetReuseAddr(ST_SOCKET socket_fd, ST_BOOLEAN reuse)             ///< 端口复用  用于服务端绑定同一端口
{
    ST_INT optval = (reuse ? 1 : 0);//0x1;
    return setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (optval_type)&optval, sizeof(optval));
}

ST_INT PortTcp::SetLinger(ST_SOCKET socket_fd, ST_INT l_onoff, ST_INT l_linger)  ///<延迟关闭连接　
{
    struct linger optLinger;
    optLinger.l_onoff = l_onoff;    //1;
    optLinger.l_linger = l_linger;  //60;
    return setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, (optval_type)&optLinger, sizeof(optLinger));
}

ST_INT PortTcp::AddMultiGroup(ST_SOCKET socket_fd, ST_CHAR* groupAddr)           ///<在指定接口上加入组播组
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(groupAddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    return setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (optval_type)&mreq, sizeof(mreq));
}
