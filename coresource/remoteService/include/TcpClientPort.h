#ifndef TCPCLIENTPORT_H
#define TCPCLIENTPORT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "datatype.h"
#include <string>
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
        bool        m_isopen = false;
        int     receModle;

        int     Open();
        void    Close();

        int     recv_Buf(ST_BYTE *bufs,int &datalen);
        //int     recv_File(char *fileName,int fileSize);

        int     send_Buf(ST_BYTE *sendbuf,int buflen);
        int     send_File(const char *fileName);
        int     rece_File(const char *fileName,int fileSize);

        string    get_dev_name();
    protected:
    private:
        int         m_sockfd;
        fd_set	    m_ReadSet;

        char        m_devname[128]; //xmlInfo
        char        m_ipaddr[64];
        int         m_port;

        unsigned  char        buf[2048];  //数据传送的缓冲区
        struct    sockaddr_in remote_addr;//服务器端网络地址结构体

};

#endif // TCPCLIENTPORT_H
