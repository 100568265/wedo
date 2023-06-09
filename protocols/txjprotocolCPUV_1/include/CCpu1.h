#ifndef CCPU1_H
#define CCPU1_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类


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

class CCpu1:public Protocol
{
    public:
        CCpu1();
        virtual ~CCpu1();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
        ST_INT countSixteen(ST_BYTE aData,ST_INT len);
        ST_INT counterBcd(ST_BYTE data_BCD);
        ST_BYTE sendbuf[256];

    protected:
    private:
};
#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CCpu1* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CCPU1_H
