#ifndef DL645_H
#define DL645_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà

void ToBCD(int Dec, uint8_t *Bcd, int length);
class CDL645 : public Protocol
{
public:
	CDL645();
	virtual ~CDL645();
    void	Init();
    void	Uninit();

    void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
	void        ReadData  (ST_UINT32 wAddr);
private:
	ST_INT m_readIndex;
    unsigned char m_addrarea[6];
    ProtocolTask m_curTask;
	bool	isYK;
	ST_BYTE sysPwd;

    int nRechargeStep;
    int m_nRechargeValue;
	void	SendYK(bool YKBit);
	void    YK1();
	void    YK2();
	void    YK3();
	void    YK4();
	void    YK7();
	void    YK8();
	void    YK9();
	void    YK10(int charge);
	void    YK11();
	void    GetRand(ST_BYTE* buf);
	ST_BYTE    GetYZ2(ST_BYTE* buf);
	ST_BYTE	DecToBCD(ST_BYTE num);
	ST_BYTE	CSCheck(ST_BYTE *buf,int len);
	uint64_t reverse_bytes(ST_BYTE *byte_array, ST_BYTE length);


	struct chargeParams{
        uint16_t chargeCount;
        uint32_t totalChargeValue;
        uint32_t acousticQuantity;
        uint32_t switchQuantity;
        uint32_t storage;
        uint32_t credit;
        uint32_t limitPower;
        uint8_t limitType;

	};

	chargeParams chargeArray[64] = {};





};


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // DL645_H
