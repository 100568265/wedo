#include "Protocol.h"     //规约父类

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class zxsf_tempdev : public Protocol
{
public:
	zxsf_tempdev();
	virtual ~zxsf_tempdev();

	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
private:

	ST_BOOLEAN m_bTask;
	ProtocolTask m_curTask;
	void SendReadParam(int Sendreaderaddr);
	void SendSetParam(int Sendreaderaddr,ST_BYTE * buf,ST_BYTE len);
	void SendSetAdjust(int Sendreaderaddr,ST_UINT16 fvalue);
	void SendSetAdjust69(int Sendreaderaddr,float fvalue);
	ST_UINT16  Wvalue;

	void TaskResult(int ret);
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
