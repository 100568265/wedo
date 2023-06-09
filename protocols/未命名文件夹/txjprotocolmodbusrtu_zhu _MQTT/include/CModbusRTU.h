#ifndef CMODBUSRTU_H
#define CMODBUSRTU_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类

#include <pthread.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dm_api.h"
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


        pthread_t g_mqtt_process_thread;
        pthread_t g_mqtt_recv_thread;
        uint8_t g_mqtt_process_thread_running = 0;
        uint8_t g_mqtt_recv_thread_running = 0;

        int32_t     res = 0;
        void       *dm_handle = NULL;
        void       *mqtt_handle = NULL;
        int        InitMQTThandle();
        int        mqisinit = -1;

        void       update_All_platform(int id,float fvalue);
        void       update_aliyun_mqtt();
    protected:
    private:
        void  SendReadCmd(ST_BYTE code, ST_UINT readAddr,ST_UINT count);
        void  SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen,ST_UINT addr);
        void  SendSingleWriteCmd(ST_FLOAT data,ST_INT readAddr,ST_INT nType);
        void  SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);
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

        ST_UINT32 htonl1(ST_UINT32 dv);
        ST_UINT GetAppWORDValue(ST_UINT v,ST_INT codetype);

        void  SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen);
        void  SendWriteCmdHex(ST_UCHAR* pData,ST_UINT dataLen);

       	ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;

        float m_devData[15];
        time_t 		Newcurtime,oldcurtime,fupdatetime;

        bool   isreport = false;

        bool   first_update = false;
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
