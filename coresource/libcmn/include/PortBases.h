#ifndef PORTBASES_H
#define PORTBASES_H

#include "datatype.h"
#include "systhread.h"
#include "sysmutex.h"
#include "syslogger.h"
#include "PortBase.h"
#include "PortTask.h"

class Channel;

class PortBases
{
public:
    PortBases(Channel *channel);
    virtual ~PortBases();
    virtual ST_VOID 				Init();
    virtual ST_VOID 				Uninit();
    virtual ST_VOID 				Work();
    virtual ST_VOID 				Stop();
    virtual ST_VOID 				Open();
    virtual ST_VOID 				Close();
	virtual ST_VOID					Close(ST_UINT64 port);
    virtual ST_BOOLEAN				IsOpened();
    virtual ST_BOOLEAN              CanRemoteCtrl();
	virtual ST_BOOLEAN				Send(ST_BYTE *pBuf,ST_INT len,ST_UINT64 dstAddr=-1);
	virtual Channel					*GetChannel();
	virtual PortBase 				*GetMainPort();
	virtual PortBase 				*GetSlavPort();
private:
    PortBase 						*m_pMainPort;
    PortBase 						*m_pSlavPort;
    Channel  						*m_pChannel;
    ST_BOOLEAN						m_sendOk;
    SysLogger						*m_pLogger;
    ST_INT							m_ConnectTimes;

};

#endif // PORTBASES_H
