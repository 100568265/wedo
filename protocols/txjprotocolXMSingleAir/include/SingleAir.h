#ifndef SINGLEAIR_H
#define SINGLEAIR_H

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
class SingleAir: public Protocol
{
    public:
        SingleAir();
        virtual ~SingleAir();
        ST_VOID	    OnRead(ST_BYTE* pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void	SendYK(ST_UINT ykAddr,bool value,bool clearerror);
		void	AskState();
		void    AskPQ();
		void	Analysis21H(ST_BYTE*pbuf);
		void	Analysis86H(ST_BYTE*pbuf);

        float  bcd2Fvalue(ST_BYTE*pbuf,int len);
		int     _counter;
		ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        int      m_readidex;
};
#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	SingleAir* CreateInstace();
#endif

#ifdef __cplusplus
}

#endif
#endif // SINGLEAIR_H
