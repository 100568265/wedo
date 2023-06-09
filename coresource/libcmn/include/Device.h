#ifndef DEVICE_H
#define DEVICE_H

#include "datatype.h"
#include "sysmutex.h"
#include "systhread.h"
#include "syslogger.h"
#include "sysstring.h"

#include "PortTask.h"
#include "ProtocolBase.h"
#include "ChannelConfig.h"
#include "TransTable.h"


#ifdef _WIN32
#else
#include <dlfcn.h>
#endif


class Channel;
class EngineBase;
class Communication;
class Dispatcher;


class Device
{
public:
    Device(Channel *channel, DeviceInfo *deviceInfo);
    virtual ~Device();
    virtual ST_VOID			Init();
    virtual ST_VOID			Uninit();
	virtual ST_VOID			Work();
    virtual ST_VOID			Stop();
    virtual ProtocolBase   *GetProtocol();
    virtual Channel		   *GetChannel();
    virtual EngineBase	   *GetEngine();
    virtual DeviceInfo	   *GetDeviceInfo();
	virtual PortBases	   *GetPorts();
	virtual ST_INT		    GetAddress();
    virtual ST_INT          GetId ();
	virtual ST_VOID			OnConnect(ST_INT port, ST_UINT64 portAddr);
	virtual ST_VOID			OnDisconnect(ST_INT port, ST_UINT64 portAddr);
protected:
    virtual ST_VOID			LoadProtocol();
public:
    virtual TransferTables &TransTables();
    TransferTables          m_transTable;
protected:
    Channel				   *m_pChannel;
    DeviceInfo			   *m_pDeviceInfo;
    EngineBase			   *m_pCEngine;
    ProtocolBase		   *m_pProtocol;
    ST_BOOLEAN			    m_Inited;
	ST_BOOLEAN			    m_Working;
    SysLogger			   *m_pLogger;
    ST_VOID				   *m_LibHandle;
};



#endif // DEVICE_H
