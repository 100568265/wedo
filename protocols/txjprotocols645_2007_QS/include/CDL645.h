#ifndef CDL645_H
#define CDL645_H

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

	void	SendYKExec(bool YKBit);
	ST_BYTE	DecToBCD(ST_BYTE num);
	ST_BYTE	CSCheck(ST_BYTE *buf,int len);

    ST_UINT modelType;

    void    singleAnalaze(ST_BYTE* pbuf,ST_INT len);
    void    threeAnalaze(ST_BYTE* pbuf,ST_INT len);
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


#endif // CDL645_H
