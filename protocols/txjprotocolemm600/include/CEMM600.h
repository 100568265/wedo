#ifndef CEMM600_H
#define CEMM600_H

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

class CEMM600 : public Protocol
{
    public:
        CEMM600();
        virtual ~CEMM600();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void SendReadCmd(ST_BYTE code,ST_UINT16 readAddr,ST_UINT16 count);
        void  SendYK(ST_BOOLEAN bIsOn);

        ST_INT       m_readIndex;
        ST_BOOLEAN   m_bTask;
        ProtocolTask m_curTask;
};


#ifdef _WIN32
	PROTOCOL_API CEMM600* CreateInstace();
#else
	CEMM600* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CEMM600_H
