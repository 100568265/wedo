#ifndef TCPCLIENTPORT_H
#define TCPCLIENTPORT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "datatype.h"
#include <string>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
typedef unsigned char 	   ST_BYTE;

using namespace std;
class TcpClientPort
{
    enum model{
        MORMALMODEL,
        SLEEPMODEL,
        SENDFILEMODEL,
        RECEFILEMODEL,
    };
    public:
        TcpClientPort(const char *addr,int port,const char *devname);
        virtual ~TcpClientPort();


        int             Open();
        void            Close();
        int             recv_Buf(ST_BYTE *bufs,int &datalen);
        int             send_Buf(ST_BYTE *sendbuf,int buflen);
        int             send_File(const char *fileName);
        int             rece_File(const char *fileName,int fileSize);
        string          get_dev_name();
        bool            isOpen();
        char *          get_Ipaddr();

    protected:
    private:
        int             m_sockfd;
        fd_set	        m_ReadSet;
        bool            m_isopen = false;
        string          m_devname; //xmlInfo
        char            m_ipaddr[64];
        int             m_port;
        unsigned  char  buf[2048];  //数据传送的缓冲区
        struct          sockaddr_in remote_addr;//服务器端网络地址结构体

        void            SetKeepalive(ST_INT socket_fd, ST_INT kpalive,ST_INT kpidle,ST_INT kpintval,ST_INT kpcout);

};

#endif // TCPCLIENTPORT_H
