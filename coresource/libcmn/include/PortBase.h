#ifndef _PORTBASE_H_
#define _PORTBASE_H_


#include "datatype.h"

#include "sysmutex.h"
#include "systhread.h"

#include "PortTask.h"

#ifdef _WIN32
#include <mstcpip.h>
#include <Ws2tcpip.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif


class Channel;
class EngineBase;
class PortBases;
class SysLogger;
class PortInfo;

typedef ST_VOID (EngineBase::* READCALLBACK)(PortTask * task);

class PortBase
{
public:
    enum Type {
        NOT_PORT = 0,
        PORT_COM = 1,
        PORT_TCPSERVER = 2,
        PORT_TCPCLIENT = 3,
        PORT_MQTT = 4,
        PORT_ICBC_MQTT = 6,
        PORT_UDP = 5,
        PORT_CAN = 8
    };

    PortBase(Channel *channel, PortInfo *portInfo);
    virtual ~PortBase();
    virtual ST_VOID				Init();
    virtual ST_VOID				Uninit();
    virtual ST_VOID				Work();
    virtual ST_VOID				Stop();
    virtual ST_VOID				Open() = 0;
    virtual ST_VOID				Close()= 0;
	virtual ST_VOID				Close(ST_UINT64 port);
    virtual ST_BOOLEAN			IsOpened();
    virtual ST_BOOLEAN			Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size) = 0;
    virtual ST_VOID				Recv() = 0;
    virtual PortInfo			*GetPortInfo();
protected:
    virtual Channel				*GetChannel();
#ifdef _WIN32
	static ST_UINT32 __stdcall	ReceiveProc(ST_VOID *param);
#else
    static ST_VOID			   *ReceiveProc(ST_VOID *param);
#endif
protected:
    Mutex						m_Mutex;
	fd_set						m_ReadSet;
    Thread						m_portThread;
    ST_BOOLEAN					m_IsOpened;
    PortInfo				   *m_pPortInfo;
    PortBases				   *m_pPorts;
	PortTask					m_portTask;
    Channel					   *m_pChannel;
    ST_BOOLEAN					m_Inited;
    ST_BYTE						m_PortBuf[1024];
	ST_BOOLEAN					m_Working;
    SysLogger				   *m_pLogger;

};

#endif // _PORTBASE_H_
