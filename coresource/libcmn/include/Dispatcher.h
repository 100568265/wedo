#ifndef ENGINE_H
#define ENGINE_H

#include "datatype.h"
#include "Channel.h"
#include "PortTask.h"


class Communication;

class Dispatcher
{
public:
    Dispatcher(Communication *communication);
    virtual ~Dispatcher();
    virtual ST_VOID			Init();
    virtual ST_VOID			Uninit();
    virtual Channel			*GetChannel(const ST_INT channelId);
    virtual ST_BOOLEAN		TransmitTask(ProtocolTask &task);
protected:
    ST_BOOLEAN				m_Inited;
    List<Channel>			*m_Channels;
    Communication			*m_pCommunication;
};

#endif // ENGINE_H
