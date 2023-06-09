#include "C103.h"
#include "Channel.h"
#include "datetime.h"

C103::C103()
{
}

C103::~C103()
{
}

void C103::Init()
{
                SendStat = 1;
                FCB = 0;
                Newcurtime = clock();
                oldcurtime = clock;
                sendserchtime() = clock();
                CLtime = sendserchtime;
                BreakCallState = 0;
                sendflag = 0;
}

void C103::Uninit()
{
}

void	C103::OnRead(ST_BYTE* pbuf, int& readed)
{

}

ST_BOOLEAN	OnSend()
{

}

ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len)
{

}

ST_BOOLEAN	IsSupportEngine(ST_INT engineType)
{
	return 1;//1为半双工
}

void	C103::SendSetframe(ST_BYTE code)
{

}

void    C103::EXpainYc(ST_BYTE* Rebuf)
{

}

void    C103::EXpainYx(ST_BYTE* Rebuf)
{

}

void    C103::EXpainBwYx(ST_BYTE* Rebuf)
{

}

void    C103::EXpainSOE(ST_BYTE* Rebuf)
{

}

void     C103::SendAllSearch()
{

}

void	C103::SendASDU21()
{

}

void    C103::ASDU2a(ST_BYTE* Rebuf)
{

}

void	C103::ASDU10(ST_BYTE* Rebuf)
{

}

void	C103::ASDU1(ST_BYTE* Rebuf)
{

}

void	C103::ASDU2(ST_BYTE* Rebuf)
{

}

void C103::SendASDU6()
{

}

void    C103::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
{

}

