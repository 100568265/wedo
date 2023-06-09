#ifndef CINNER_H
#define CINNER_H

#include "Protocol.h"     //��Լ����
#include "DataCache.h"    //���ݻ�����
#include "Device.h"       //ͨѸ�豸��
#include "Devices.h"      //ͨѸ�豸������
#include "sysinifile.h"   //INI�ļ���ȡ��
#include "datatype.h" //
#include "rtobjecttree.h" //

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

class CInner : public Protocol
{
    public:
        CInner();
        virtual ~CInner();
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

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_BYTE bySendbuf[1024];
        double preValue[1024][1024];
        ST_BOOLEAN m_Begin[1024];
        ST_BOOLEAN is_totalcall;
        ST_BOOLEAN is_initial;
        ST_INT nsendfailnum;
 };

#ifdef _WIN32
	PROTOCOL_API CInner* CreateInstace();
#else
	CInner* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CINNER_H
