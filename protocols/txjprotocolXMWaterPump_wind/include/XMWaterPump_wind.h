#ifndef XMWATERPUMP_WIND_H
#define XMWATERPUMP_WIND_H

#include "Protocol.h"

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
class XMWaterPump_wind: public Protocol
{
    public:
        XMWaterPump_wind();
        virtual ~XMWaterPump_wind();
        ST_VOID	    OnRead(ST_BYTE* pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void	SendYK(ST_UINT ykAddr,bool value,bool clearerror);
		void	AskState();
		void	Analysis21H(ST_BYTE*pbuf);

        float  bcd2Fvalue(ST_BYTE*pbuf,int len);
		int     _counter;
		ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	XMWaterPump_wind* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // XMWATERPUMP_WIND_H
