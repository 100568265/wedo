#ifndef TUOBAO_TCPCLIENT_H
#define TUOBAO_TCPCLIENT_H

#include <netinet/in.h>
#include <sys/socket.h>

typedef struct _tuobao_tcpclient{
    int socket;
    int remote_port;
    char remote_ip[16];
    struct sockaddr_in _addr;
    int connected;
    } tuobao_tcpclient;

class ctuobao_tcpclient
{

    public:
        ctuobao_tcpclient();
        virtual ~ctuobao_tcpclient();
        int tuobao_tcpclient_create(tuobao_tcpclient *,const char *host, int port);
        int tuobao_tcpclient_conn(tuobao_tcpclient *);
        int tuobao_tcpclient_recv(tuobao_tcpclient *,char **lpbuff,int size);
        int tuobao_tcpclient_send(tuobao_tcpclient *,char *buff,int size);
        int tuobao_tcpclient_close(tuobao_tcpclient *);

        int http_post(tuobao_tcpclient *pclient,char *page,char *request,char **response);
    protected:
    private:
};

#endif // TUOBAO_TCPCLIENT_H
