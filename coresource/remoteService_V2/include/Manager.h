#ifndef MANAGER_H
#define MANAGER_H
#include<stdio.h>
#include<time.h>
#include "Thread.h"

class TcpClientPort;
class remoteProtocol;
class Manager
{
    public:
        Manager();
        virtual ~Manager();

        int     load_configs();
        int     start();

        time_t  rec_Time,new_Time;
        bool    startMonitor(int port);
        void    transfer_Monitor();
        void    Recive_transfer();
    protected:
        static ST_VOID	    *MonitorTaskProc(ST_VOID *param);
        Thread              m_MonitorThread;
        static ST_VOID	    *ReceiveTaskProc(ST_VOID *param);
        Thread              m_ReceiveThread;
    private:
        TcpClientPort   *m_client;
        remoteProtocol  *m_protocol;

        TcpClientPort   *m_MonitorClient;//报文监听客户端
        TcpClientPort   *m_tranferClient;//报文转发客户端
        void            onProcess();
        void            checkLastTime();
        char            m_devName[128];
        char            m_ipaddr[64];
        int             m_port;

};

#endif // MANAGER_H
