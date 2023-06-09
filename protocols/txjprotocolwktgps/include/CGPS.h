#ifndef CGPS_H
#define CGPS_H

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


class CGPS : public Protocol
{
    public:
        CGPS();
        virtual ~CGPS();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_BOOLEAN  curEngineType;
};


#ifdef _WIN32
	PROTOCOL_API CGPS* CreateInstace();
#else
	CGPS* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CGPS_H
