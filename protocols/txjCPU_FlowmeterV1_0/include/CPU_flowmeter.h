#ifndef CPU_FLOWMETER_H
#define CPU_FLOWMETER_H

#include "Protocol.h"    //规约父类
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


class CPU_flowmeter : public Protocol
{
    public:
        CPU_flowmeter();
        virtual ~CPU_flowmeter();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
        ST_DOUBLE ChangeFloat(ST_BYTE* pbuf, ST_INT len);
        ST_DOUBLE ChangeInt(ST_BYTE *pbuf,ST_INT len);
    protected:
    private:
        ST_DOUBLE datavalue;
};

#ifdef _WIN32
	PROTOCOL_API CPU_flowmeter* CreateInstace();
#else
	CPU_flowmeter* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CPU_FLOWMETER_H
