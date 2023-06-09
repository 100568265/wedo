
#ifndef __YD800_H__
#define __YD800_H__

#include "Protocol.h"

typedef struct DataAreaItem ST_DataAreaItem;

class YD800 : public Protocol
{
public:
	YD800();
	~YD800();

	void	Init();
	void	Uninit();

	void	OnRead(ST_BYTE * pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

	void	YKSelect (ProtocolTask & task);
	void	YKExecut (ProtocolTask & task);
	void	YKCancel (ProtocolTask & task);
private:

	void	SendASDU21();

	void	SendASDU6();
//	void    ASDU2a(ST_BYTE* Rebuf);
//	void    ASDU1(ST_BYTE* Rebuf);
//	void	ASDU10(ST_BYTE* Rebuf);
//	void    ASDU2(ST_BYTE* Rebuf);

	void	SendShortFrame(ST_BYTE code);
	// void    EXpainYc(ST_BYTE* Rebuf);
	// void    EXpainYx(ST_BYTE* Rebuf);
	// void    EXpainBwYx(ST_BYTE* Rebuf);
	// void    EXpainSOE(ST_BYTE* Rebuf);

	void    AskASDU0x15 (ST_BYTE rii, ST_UINT16 gin, ST_BYTE kod);

	void    SendAllSearch();

	void	AnalysisASDU0x05 (ST_BYTE * data);
	void	AnalysisASDU0x09 (ST_BYTE * data);
	void	AnalysisASDU0x0A (ST_BYTE * data);
//	void	AnalysisASDU15 (ST_BYTE * data);
	void	AnalysisASDU0x2A (ST_BYTE * data);

	void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);

	inline void	StateTransition(ST_BYTE state)
	{ _old_state = _sendstate; _sendstate = state; }

    ST_BOOLEAN m_bTask;
	ProtocolTask m_curTask;

	ST_BYTE    newday, oldday, sendflag;
    clock_t	   oldcurtime, _askserachtime, CLtime;

	ST_BOOLEAN _has1stData;
	ST_BYTE    _sendstate;
	ST_BYTE    _old_state;
	ST_BYTE    FCB;
	ST_BYTE    BreakCallState;

};

_PROTOCOL_INTERFACE_DECLARATION_(YD800);

#endif // __YD800_H__
