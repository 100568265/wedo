#ifndef CMODBUSRTU_H
#define CMODBUSRTU_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include <list>

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
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

        bool    CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);
        void    SendDIStateData       (ST_BYTE * pbuf, ST_INT len);
        void    SendInputRegisterData (ST_BYTE * pbuf, ST_INT len);
        void    SendKeepRegisterData  (ST_BYTE * pbuf, ST_INT len);
        void    WriteTimeDate         (ST_BYTE * pbuf, ST_INT len);
        void    ErrorResponse (ST_BYTE fc, ST_BYTE ec);

        void    SendHistoryData    (ST_BYTE * pbuf, ST_INT len);
        void    GetXMLData(int x_day,int x_hour,int x_min);

     	ST_BOOLEAN    m_bTask;
        ProtocolTask  m_curTask;
        list<float>   vLists;
        bool          curEngineType;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CModbusRTU* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CMODBUSRTU_H
