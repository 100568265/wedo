#ifndef CMODBUSRTU_H
#define CMODBUSRTU_H

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

using namespace std;

class CModbusRTU : public Protocol
{
    public:
        CModbusRTU();
        virtual ~CModbusRTU();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:

        bool    CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);
        void    SendDIStateData       (ST_BYTE * pbuf, ST_INT len);
        void    SendInputRegisterData (ST_BYTE * pbuf, ST_INT len);
        void    SendKeepRegisterData  (ST_BYTE * pbuf, ST_INT len);
        void    TaskHandlerToLower    (ST_BYTE *pbuf,ST_INT len);  //�ظ�ң�ر���
        ST_BOOLEAN    TransToLower          (ST_BYTE *pbuf);//��ң�������·����豸
        void    ErrorResponse         (ST_BYTE fc, ST_BYTE ec);

     	ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;

        bool curEngineType;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CModbusRTU* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CMODBUSRTU_H
