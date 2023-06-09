#ifndef CCHANGYANG_H
#define CCHANGYANG_H
#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include  <time.h>


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus

#define BIGLEN  (255)

extern "C"
{
#endif
using namespace std;
class Cchangyang : public Protocol
{
    public:
        Cchangyang();
        virtual ~Cchangyang();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

        void  Confirmframe();//召唤确认帧
        void  CallAll();//总召唤
        void  EndCallAll();//结束总召唤
        void  HeartBeat();//心跳报文
        void  AdjustTime();

    protected:
    private:
        ST_BYTE SendState;
        ST_BYTE code;

};
#ifdef _WIN32
	PROTOCOL_API Cchangyang* CreateInstace();
#else
	Cchangyang* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CCHANGYANG_H
