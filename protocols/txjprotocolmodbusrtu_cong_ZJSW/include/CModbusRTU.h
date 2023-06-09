#ifndef CMODBUSRTU_H
#define CMODBUSRTU_H

#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà
#include <list>
//#include "tinyxml.h"
#include "DataCenter.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

using namespace std;
struct xmlData{
        int areaId;
        int dataId;
        char itemName;
        float fvalue;
    };

typedef list<xmlData> LISTXmlData;
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

        void    ErrorResponse (ST_BYTE fc, ST_BYTE ec);
        void    TaskHandlerToLower    (ST_BYTE *pbuf,ST_INT len);  //回复遥控报文
        ST_BOOLEAN    TransToLower          (ST_BYTE *pbuf);//将遥控任务下发到设备

     	ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;

        bool        curEngineType;
        void        checkTime(ST_BYTE *pbuf);
        void        CTRespon();

        void        GetHistoryData(ST_BYTE *pbuf);
        void        getbefordayString(int bTime,char *dest);
        LISTXmlData dataList;

        void        assemblydata(ST_BYTE *dbuf);
        ST_FLOAT    getfData2tableList(int index);
        ST_DOUBLE   getdData2tableList(int index);
        void        calculationValue(double d1,double d2,double d3,double d4,ST_BYTE *dpbuf);
        void        waterStatus(int pos,ST_BYTE *pbuf);
        void        fv2UIntt16(float fvalue,ST_BYTE *pbuf);
        void        fv2UIntt32(float fvalue,ST_BYTE *pbuf);
        void        dv2UIntt32(double dvalue,ST_BYTE *pbuf);

        DataCenter  *datacenter;
        ST_BYTE     m_historyData[256];
        void        SendHistoryRegisterData (ST_BYTE * pbuf, ST_INT len);
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
