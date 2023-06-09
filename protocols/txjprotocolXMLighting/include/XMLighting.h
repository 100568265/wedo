#ifndef XMLIGHTING_H
#define XMLIGHTING_H
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

class XMLighting : public Protocol
{
    enum FUNCODE{
		read_dev_pro = 0x01,
		read_dev_addr= 0x02,
		set_dev_addr = 0x03,
		check_time =   0x04,
		set_dev_time = 0x05,
		read_dev_AcPower = 0x06,    //查询设备有功电度
		read_dev_app_power = 0x0F,  //读取视在功率
		req_control_AirCon = 0x10,
		read_status_AirCon = 0x11,
		controal_AirCon    = 0x20
	};
    public:
        XMLighting();
        virtual ~XMLighting();

        ST_VOID	    OnRead(ST_BYTE* pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    protected:
    private:

        void	SendYK(ST_UINT ykAddr,bool value,bool clearerror);
		void	AskState();
		void	Analysis21H(ST_BYTE*pbuf);

		int     _counter;
		ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	XMLighting* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // XMLIGHTING_H
