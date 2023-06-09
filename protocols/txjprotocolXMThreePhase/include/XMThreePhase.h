#ifndef XMTHREEPHASE_H
#define XMTHREEPHASE_H

#include "Protocol.h"

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

class XMThreePhase: public Protocol
{
    public:
        XMThreePhase();
        virtual ~XMThreePhase();

        ST_VOID	    OnRead(ST_BYTE* pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void	SendYK(ST_UINT ykAddr,bool value,bool clearerror);
		void	AskState();
		void	Analysis21H(ST_BYTE*pbuf);

        float  bcd2Fvalue(ST_BYTE*pbuf,int len);
		int     _counter;
		ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	XMThreePhase* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // XMTHREEPHASE_H
