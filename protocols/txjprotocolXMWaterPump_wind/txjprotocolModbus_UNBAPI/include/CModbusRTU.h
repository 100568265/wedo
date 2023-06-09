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
        void  SendReadCmd(ST_BYTE code, ST_UINT readAddr,ST_UINT count);
        void  SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen,ST_UINT addr);
        void  SendSingleWriteCmd(ST_FLOAT data,ST_INT readAddr,ST_INT nType);
        // void  SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);
        void  SendYK(ST_UINT writeAddr,ST_INT value);
        void  SendPreYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);
        void  SendYT(ST_UINT writeAddr,ST_UINT wIndex,ST_BOOLEAN bIsOn);
        void  ProcessMemory(ST_BYTE* buf,ST_BYTE count);

/*        void  ProcessMemorySByte    (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryInt16    (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryInt32    (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryByte     (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryUInt16   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryUInt32   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemorySingle   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryBoolean  (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryString   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryBytes    (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryDouble   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryDecimal  (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryDateTime (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryInt64    (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);
        void  ProcessMemoryUInt64   (ST_DeviceDataArea *devicedataarea, ST_BYTE* buf, ST_INT ncurreadIndex, ST_INT k);*/

        void  ProcessMemorySByte   (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryInt16   (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryInt32   (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryByte    (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryUInt16  (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryUInt32  (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemorySingle  (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryBoolean (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryString  (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryBytes   (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryDouble  (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryDecimal (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryDateTime(ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryInt64   (ST_BYTE*, const ST_DataAreaItem&);
        void  ProcessMemoryUInt64  (ST_BYTE*, const ST_DataAreaItem&);

        void FillWORD(ST_BYTE* buf,ST_UINT v);
        void SendToDev(ST_BYTE* buf, ST_INT len);


        ST_UINT32 htonl1(ST_UINT32 dv);
        ST_UINT GetAppWORDValue(ST_UINT v,ST_INT codetype);

        void  SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen);
        void  SendWriteCmdHex(ST_UCHAR* pData,ST_UINT dataLen);

       	ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;
        ST_INT16 snCnt;

        const ST_BYTE UNBAPI_VERSION = 0x00;
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
