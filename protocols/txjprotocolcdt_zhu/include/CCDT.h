#ifndef CCDT_H
#define CCDT_H

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

class CCDT : public Protocol
{
    public:
        CCDT();
        virtual ~CCDT();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_BOOLEAN  VerifyCRC(ST_BYTE *p);

        ST_BOOLEAN  curEngineType;
};

#ifdef _WIN32
	PROTOCOL_API CCDT* CreateInstace();
#else
	CCDT* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CCDT_H
