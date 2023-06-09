#ifndef C103_H
#define C103_H

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include <stdio.h>

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif
typedef unsigned short      WORD;
class C103 : public Protocol
{

public:
	C103();
	virtual ~C103();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
	time_t Newcurtime,oldcurtime,sendserchtime,CLtime,m_ivRead;//clock_t		;
	time_t     t1,t2;
	void	SendASDU21();
	void	SendASDU6();
	void    ASDU2a(ST_BYTE* Rebuf);
	void    ASDU1(ST_BYTE* Rebuf);
	void	ASDU10(ST_BYTE* Rebuf);
	void    ASDU2(ST_BYTE* Rebuf);
	ProtocolTask m_curTask;
	ST_BYTE    newday,oldday,sendflag;

	void  SendPreYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn);//遥控选择
	void  SendYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn);//遥控执行
	void  SendEndYK(ST_BYTE fc, WORD wAddr, ST_BOOLEAN bYkOn);//遥控结束

	enum YKStep
	{
		undefined = 0,
		SEND_PRE_YK,
		SEND_PRE_YK_END,
		SEND_YK
	};
private:
	void	SendSetframe(ST_BYTE code);
	ST_BYTE    SendState;
	ST_BYTE    FCB;
	ST_BYTE    BreakCallState;
	void    EXpainYc(ST_BYTE* Rebuf);
	void    EXpainYx(ST_BYTE* Rebuf);
	void    EXpainBwYx(ST_BYTE* Rebuf);
	void    EXpainSOE(ST_BYTE* Rebuf);
	void    SendAllSearch();

	void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API C103* CreateInstace();
#else
	C103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C103_H
