
#ifndef __PASSTHROUGH_H__
#define __PASSTHROUGH_H__

#include "Protocol.h"

class PassThrough : public Protocol
{
public:
	PassThrough();
	virtual ~PassThrough();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

private:

    ProtocolTask _task;
}; 

_PROTOCOL_INTERFACE_DECLARATION_(Passthrough)

#endif // __PASSTHROUGH_H__