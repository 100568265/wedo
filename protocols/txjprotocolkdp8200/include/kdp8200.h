
#ifndef _KDP_8200_H_
#define _KDP_8200_H_

#include "Protocol.h"     //规约父类

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

class CKDP8200 : public Protocol
{
public:
	CKDP8200();
	virtual ~CKDP8200();

	void	Init();
	void	Uninit();

	void	    OnRead(ST_BYTE* pbuf,ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
protected:
	void SendEx   (ST_BYTE* data);
	void SendCall (ST_BYTE  code);

	void TaskHandlerToLower (const ProtocolTask& task);
	void TaskHandlerToUpper (const ST_BYTE* data);

	void AnalyzerForSOE  (const ST_BYTE* data);
	void AnalyzerForYXYC (const ST_BYTE* data);

	ST_BOOLEAN   m_bIsSendSuc;
	ST_BOOLEAN   m_bTask;
	ProtocolTask m_curTask;
	unsigned int m_sendindex;

	ST_BOOLEAN   m_is_syc_clock;
	time_t       m_lasttime;
private:
};

#ifdef _WIN32
	PROTOCOL_API CKDP8200* CreateInstace();
#else
	CKDP8200* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // _KDP_8200_H_
