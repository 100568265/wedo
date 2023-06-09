#ifndef CMODBUSRTU_H
#define CMODBUSRTU_H
#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà

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
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;
        ST_INT countDataNum;
        float countAllData;
        ST_INT th_count;
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CModbusRTU* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CMODBUSRTU_H


