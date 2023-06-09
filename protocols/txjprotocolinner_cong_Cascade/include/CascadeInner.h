#pragma once

#include "Protocol.h"     //��Լ����
#include "DataCache.h"    //���ݻ�����
#include "Device.h"       //ͨѸ�豸��
#include "Devices.h"      //ͨѸ�豸������
#include "sysinifile.h"   //INI�ļ���ȡ��
#include "datatype.h" //
#include "rtobjecttree.h" //
#include <vector>

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class NodeTree;

struct varNode{
    ST_DUADDR *duaddr;
    string     nodeName;
};

class CascadeInner : public Protocol
{
    public:
        CascadeInner();
        virtual ~CascadeInner();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void    SendData();
        void    SendSOE(ProtocolTask& curTask);
        void    SendYKEcho(ProtocolTask& curTask);
        void    SendYTEcho(ProtocolTask&);

        bool        CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);
        void        InitTransfertable();
        void        GetDitNameByAddr(DeviceInfo *devinfo,int id,ST_CHAR* ditName);
        int         InitDevInfo();

        bool        is_init;
        vector<varNode>  vec_Node;
        std::string m_devName;

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_BYTE bySendbuf[1024];
        //double preValue[1024][1024];
        ST_BOOLEAN m_Begin[1024];
        ST_BOOLEAN is_totalcall;
        ST_BOOLEAN is_initial;
        ST_INT nsendfailnum;
 };

#ifdef _WIN32
	PROTOCOL_API CascadeInner* CreateInstace();
#else
	CascadeInner* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif


