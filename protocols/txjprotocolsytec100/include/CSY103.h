#ifndef CSY103_H
#define CSY103_H

#include "Protocol.h"     //��Լ����
#include "DataCache.h"    //���ݻ�����
#include "Device.h"       //ͨѸ�豸��
#include "Devices.h"      //ͨѸ�豸������
#include "sysinifile.h"   //INI�ļ���ȡ��


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

typedef struct
{
	ST_BYTE FUN:4; //������
	ST_BYTE FCV:1; //֡����λ��Ч
	ST_BYTE FCB:1;//֡����λ
	ST_BYTE PRM:1;//��������
	ST_BYTE DIR:1;//����
}SENDLINKCODE;

class CSY103 : public Protocol
{
    public:
        CSY103();
        virtual ~CSY103();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

        void    ExplainClass2(ST_BYTE* pbuf,ST_INT nlen); //����2������
        void    ExplainCallAll(ST_BYTE* pbuf,ST_INT nlen);//�������ٻ�
        void    ExplainEvent(ST_BYTE* pbuf,ST_INT nlen);//������λ�¼�
        void    ExplainFixValue(ST_BYTE* pbuf,ST_INT nlen);//������ֵ
        void    ExplainSystemFixValue(ST_BYTE* pbuf,ST_INT nlen);//������ֵ


        void    LinkReset(void);//��λ��·
        void    CallAll(void);//���ٻ���ʼ
        void    CallClass1Data(void);//�ٻ�һ������
        void    CallClass2Data(void);//�ٻ���������
        void    SendTime(void);//��ʱ
        void    Resetdevice(void);//����װ��
        void    ReStart(void);//��������
        ST_BOOLEAN    YKSelect(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    YKExecut(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    YKCancel(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    ReadFixValue(ST_INT nfixIndex,ST_INT nfixnum);
        ST_BOOLEAN    WriteFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask);
        ST_BOOLEAN    ReadSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum);
        ST_BOOLEAN    WriteSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask);

//
        ST_BYTE GetCHKSUM(ST_BYTE* pbuf,ST_BYTE bylen);//У��
        bool    m_bFCB;//֡����λ
        bool    m_bresetlink;//��λ��·
        bool    m_bcallall;//�������ٻ�����
        bool    m_bcallclass1;//���ٻ�һ������
        bool    m_bcallclass2;//���ٻ���������
        bool    m_blink;//��·״��

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT  m_nroute;
        ST_BYTE m_byOnOff;
        struct timeval  m_Timeout;
        time_t m_tlast;
    protected:
    private:
};

#ifdef _WIN32
	PROTOCOL_API CSY103* CreateInstace();
#else
	CSY103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CSY103_H
