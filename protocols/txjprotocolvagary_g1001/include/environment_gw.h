
#ifndef __ENVIRONMENT_GW_H__
#define __ENVIRONMENT_GW_H__

#include "Protocol.h"     //规约父类

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class EnvironmentGw : public Protocol
{
public:
	EnvironmentGw();
	virtual ~EnvironmentGw();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

	ST_BOOLEAN SendString(const char * data);

	void SendAskYC ();

	inline ST_BOOLEAN TransferTask (const ProtocolTask & task)
		{ return this->Transfer((ProtocolTask*)&task); }

private:

//	void	EXplainValue(ST_BYTE *pbuf, int len);

	int32_t _chlid;
	int32_t _devsn;
	int32_t _interval;

	int32_t _sendindex;

	ProtocolTask _task;

	bool   _isread;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API EnvironmentGw* CreateInstace();
#else
	EnvironmentGw* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif //__ENVIRONMENT_GW_H__