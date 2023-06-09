#ifndef DEVICES_H
#define DEVICES_H

#include "datatype.h"
#include "Device.h"

class Channel;

class Devices
{
public:
    Devices(Channel *channel);
    virtual ~Devices();
    virtual ST_VOID			Init();
    virtual ST_VOID			Uninit();
	virtual ST_VOID			Work();
	virtual ST_VOID			Stop();
	virtual ST_INT			GetCount();
    virtual Device			*GetDevice(ST_INT deviceId);
	virtual Device			*GetDeviceByAddr(ST_INT deviceAddr);
	virtual ST_VOID			OnConnect(ST_INT port,ST_UINT64 portAddr);
	virtual ST_VOID			OnDisconnect(ST_INT port,ST_UINT64 portAddr);
protected:
    List<Device>			*m_pDevices;
private:
	ST_BOOLEAN				m_Working;
    Channel					*m_pChannel;
	SysLogger				*m_pLogger;
};

#endif // DEVICES_H
