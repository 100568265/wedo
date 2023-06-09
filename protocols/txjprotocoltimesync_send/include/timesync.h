
#ifndef _GPS_ZHU_H_
#define _GPS_ZHU_H_

#include "Protocol.h"     //规约父类
#include "datatype.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class TimeSync : public Protocol
{
public:
	TimeSync();
	virtual ~TimeSync();
	
    void	Init();
    void	Uninit();
    void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
    ST_BOOLEAN	OnSend();
    ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
    ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
protected:
private:
    ST_BOOLEAN  curEngineType;
};

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif //_WIN32

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_GPS_ZHU_H_