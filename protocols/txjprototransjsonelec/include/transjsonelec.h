
#ifndef __TRANS_JSON_ELEC_H__
#define __TRANS_JSON_ELEC_H__

#include "Protocol.h"

class TransJsonelec : public Protocol
{
public:
    TransJsonelec();
	virtual ~TransJsonelec();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

};

_PROTOCOL_INTERFACE_DECLARATION_(TransJsonelec)

#endif // __TRANS_JSON_ELEC_H__