
#ifndef _XUJI103_H_
#define _XUJI103_H_

#include "Protocol.h"

class XUJI103 : public Protocol
{
	XUJI103(const XUJI103&);
	XUJI103& operator= (const XUJI103&);
public:
	XUJI103();
	~XUJI103();
	
	void Init();
	void Uninit();

    void	   OnRead(ST_BYTE* pbuf,ST_INT& readed);
    ST_BOOLEAN OnSend();
    bool	   OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	   IsSupportEngine(ST_INT engineType);
};

_PROTOCOL_INTERFACE_DECLARATION_(XUJI103)

#endif //_XUJI103_H_