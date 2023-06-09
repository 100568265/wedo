#ifndef MODBUS_H
#define MODBUS_H

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
class modbus : public Protocol
{
    public:
        modbus();
        virtual ~modbus();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
        modbus* GetDevUnit(unsigned long byID);
    protected:
    private:
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	modbus* CreateInstace();
#endif

#ifdef __cplusplus
}

#endif
#endif // MODBUS_H
