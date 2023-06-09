
#ifndef __ICBC_IOT_H__
#define __ICBC_IOT_H__


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif


#include "Protocol.h"

class ICBC_IOT : public Protocol
{
public:
	ICBC_IOT();
	~ICBC_IOT();
	
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
private:

	int index_;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // __ICBC_IOT_H__