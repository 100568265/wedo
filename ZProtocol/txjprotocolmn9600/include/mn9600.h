#ifndef MN9600_H
#define MN9600_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

class mn9600 : public Protocol
{
                public:
                                mn9600();
                                virtual ~mn9600();
                                void Init();
                                void Uninit();

                                void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
                                ST_BOOLEAN	OnSend();
                                ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
                                ST_BOOLEAN  IsSupportEngine (ST_INT IsSupportEngine);



                                ST_BYTE ReadIndex;
                                ST_BYTE sendbuf[256];

                public:
                                ST_BOOLEAN m_bTask;
                                ProtocolTask m_curTask;
                                ST_INT m_curreadIndex;
                                ST_INT m_readIndex;

                                void SendYK(ST_BOOLEAN bIsOn);
                                void ConfirmYK(ST_BOOLEAN bIsOn);
                                void CancelYK(ST_BOOLEAN bIsOn);

                                void ReadData1();
                                void ReadData2();
                                void ReadData3();
                                void ReadData4();

                                void EXpainYx(ST_BYTE* pbuf);
                                void EXpainYc(ST_BYTE* pbuf);
                                void EXpainEq(ST_BYTE* pbuf);
                                void EXpainSOE(ST_BYTE* pbuf);

                                void TransferEx (float value[ ], ST_BYTE year, ST_BYTE month, ST_BYTE day, ST_BYTE hour, ST_BYTE minute, ST_BYTE second, ST_UINT16 msec);


                protected:

                private:
};

#ifdef _WIN32
	PROTOCOL_API mn9600* CreateInstace();
#else
	mn9600* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif


#endif // MN9600_H
