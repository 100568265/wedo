
#ifndef __MQTT_PORT_ICBC_H__
#define __MQTT_PORT_ICBC_H__

#include "PortBase.h"

#include "boost/scoped_ptr.hpp"

namespace MQTT {

class PortICBC : public PortBase
{
public:
	PortICBC(Channel *channel, PortInfo *portInfo);
	virtual ~PortICBC();
    ST_VOID					Init  ();
    ST_VOID					Uninit();
    ST_VOID					Open  ();
    ST_VOID					Close ();
    ST_BOOLEAN				Send  (ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size);
    ST_BOOLEAN				Send  (ST_UINT64 portAddr, char * pBuf, ST_UINT size);
    ST_VOID					Recv  ();

private:
	boost::scoped_ptr<class PortICBCImpl> impl_;
};
}; // namespace MQTT

#endif // __MQTT_PORT_ICBC_H__
