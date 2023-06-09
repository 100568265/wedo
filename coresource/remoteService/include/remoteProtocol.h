#ifndef REMOTEPROTOCOL_H
#define REMOTEPROTOCOL_H
#include "datatype.h"
#include <string>
#include <string.h>

class TcpClientPort;
class remoteProtocol
{
    public:
        remoteProtocol(TcpClientPort *client);
        virtual ~remoteProtocol();

        TcpClientPort   *m_client;

        void    process_receive(ST_BYTE *bpuf,int dataLen);
    protected:
    private:
        int     m_fileID;

        int     m_recFID;
        int     m_recfileSize;

        int     onRead(ST_BYTE *bpuf,int &read);
        int     replay_Server(ST_BYTE* bpuf);
        int     Send(ST_BYTE *pbuf,int len);

        void    parsing_01(ST_BYTE *pbuf);
        void    parsing_02(ST_BYTE *pbuf);
        void    parsing_03(ST_BYTE *pbuf);

        void    request_resend();
        void    heart_beat();
        void    send_devName();
        void    send_fileSize(int fID);
        TcpClientPort   *m_MonitorClient;

};

#endif // REMOTEPROTOCOL_H
