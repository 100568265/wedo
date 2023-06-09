#ifndef MANAGER_H
#define MANAGER_H
#include<stdio.h>
#include<time.h>

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


    protected:
    private:
        TcpClientPort   *m_client;
        remoteProtocol  *m_protocol;

        void            checkLastTime();

        char            m_devName[128];
        char            m_ipaddr[64];
        int             m_port;
};

#endif // MANAGER_H
