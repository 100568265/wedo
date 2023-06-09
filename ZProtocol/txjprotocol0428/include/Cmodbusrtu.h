#ifndef CMODBUSRTU_H//表示头文件只调用一次
#define CMODBUSRTU_H

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


class Cmodbusrtu
{
                public:
                                Cmodbusrtu();
                                virtual ~Cmodbusrtu();
                                void	Init();
                                void	Uninit();
                                void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
                                ST_BOOLEAN	OnSend();
                                ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
                                ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
                private:
                                void  SendReadCmd(ST_BYTE code, ST_UINT readAddr,ST_UINT count);
                                void  SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen,ST_UINT addr);
                                void  SendSingleWriteCmd(ST_FLOAT data,ST_INT readAddr,ST_INT nType);
                                void  SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);
                                void  SendPreYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);
                                void  SendYT(ST_UINT writeAddr,ST_UINT wIndex,ST_BOOLEAN bIsOn);

                                void  ProcessMemory(ST_BYTE* buf,ST_BYTE count);
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
};

#ifdef _WIN32
	PROTOCOL_API Cmodbusrtu* CreateInstace();
#else
	Cmodbusrtu* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif


#endif // CMODBUSRTU_H
