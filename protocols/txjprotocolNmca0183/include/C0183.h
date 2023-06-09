#ifndef C0183_H
#define C0183_H
#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include <cmath>

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

class C0183 : public Protocol
{
    public:
        C0183();
        virtual ~C0183();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        bool	outOfChina(double lat, double lon);
        double  transformLat(double x, double y);
        double  transformLon(double x, double y);
        void	transform2Mars(double wgLat, double wgLon,double &mgLat,double &mgLon);
        void	TranslateCoordilate(long double latitude,long double longitude,int *buf);
        void	ToDec(int *buf,double &x, double &y);
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	C0183* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C0183_H
