#ifndef PORTTCP_H
#define PORTTCP_H


#include "datatype.h"
#include "PortBase.h"

#ifdef _WIN32
	typedef SOCKET ST_SOCKET;
#else
	typedef ST_INT ST_SOCKET;
#endif

class PortTcp
{
public:
    PortTcp();
    virtual ~PortTcp();
    ST_VOID		SetKeepalive  (ST_SOCKET socket_fd, ST_INT kpalive,ST_INT kpidle,ST_INT kpintval,ST_INT kpcout);
    ST_INT		SetNonBlocking(ST_SOCKET socket_fd);
    ST_INT		SetLinger     (ST_SOCKET socket_fd, ST_INT l_onoff,ST_INT l_linger);
    ST_INT		SetReuseAddr  (ST_SOCKET socket_fd, ST_BOOLEAN reuse);
    ST_INT		AddMultiGroup (ST_SOCKET socket_fd, ST_CHAR* groupAddr);
};
#endif // PORTTCP_H
