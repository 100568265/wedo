#ifndef PORTLGMQTT_H
#define PORTLGMQTT_H

#include "PortBase.h"

#include "boost/scoped_ptr.hpp"

namespace LGMQTT {

class PortLGMQTT : public PortBase
{
public:
	PortLGMQTT(Channel *channel, PortInfo *portInfo);
	virtual ~PortLGMQTT();
    ST_VOID					Init  ();
    ST_VOID					Uninit();
    ST_VOID					Open  ();
    ST_VOID					Close ();
    ST_BOOLEAN				Send  (ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size);
    ST_VOID					Recv  ();

private:
	boost::scoped_ptr<class PortLGImpl> impl_;
};
}; // namespace LGMQTT
#endif // PORTLGMQTT_H
