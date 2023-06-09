
#ifndef __GzqxCentralCtrl_H__
#define __GzqxCentralCtrl_H__

#include "Protocol.h"

class GzqxCentralCtrl : public Protocol
{
public:
	GzqxCentralCtrl();
	virtual ~GzqxCentralCtrl();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    void    tranferNewTask(bool bIsOn);
private:

    ProtocolTask _task;
};

_PROTOCOL_INTERFACE_DECLARATION_(GzqxCentralCtrl)

#endif // __GzqxCentralCtrl_H__
