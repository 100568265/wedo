#ifndef CTCM100_H
#define CTCM100_H

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

class Ctcm100 : public Protocol
{
    public:
        Ctcm100();
        virtual ~Ctcm100();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void SendReadCmd(ST_BYTE code,ST_UINT16 readAddr,ST_UINT16 count);
};


#ifdef _WIN32
	PROTOCOL_API Ctcm100* CreateInstace();
#else
	Ctcm100* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CTCM100_H
