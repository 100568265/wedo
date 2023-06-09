#ifndef REMOTEPROTOCOL_H
#define REMOTEPROTOCOL_H
#include "datatype.h"
#include <string>
#include <string.h>
#include <map>
#include "Thread.h"

using namespace std;
class TcpClientPort;
class Manager;
class remoteProtocol
{
    public:
        remoteProtocol(Manager *manager,TcpClientPort *client);
        virtual ~remoteProtocol();

        void                process_receive(ST_BYTE *bpuf,int dataLen);

    protected:
    private:
        string              m_filename;
        map<string,string>  m_filepathMap;
        TcpClientPort       *m_client;
        Manager             *m_manager;



        int                 onRead(ST_BYTE *bpuf,int &read);
        int                 replay_Server(ST_BYTE* bpuf);
        int                 Send(ST_BYTE *pbuf,int len);
        void                parsing_01(ST_BYTE *pbuf);
        void                parsing_02(ST_BYTE *pbuf);
        void                parsing_03(ST_BYTE *pbuf);
        void                request_resend();
        void                heart_beat();
        void                send_devName();
        void                send_fileSize(string filePath);
        void                InitFilePathMap();
        string              getfilePath(string filename);
        bool                coverFile();//覆盖文件
};

#endif // REMOTEPROTOCOL_H
