#ifndef PORTTASK_H
#define PORTTASK_H

#include "datatype.h"

#ifdef _WIN32
class COMMUNICATIONDLL_API  PortTask
#else
class  PortTask
#endif
{
public:
    PortTask();
    PortTask(const PortTask &task);
    virtual ~PortTask();
    ST_BYTE				   *GetBuffer();
    ST_INT					GetBufferLen();
    ST_VOID					Write(ST_BYTE * buf, ST_INT size);
public:
    ST_INT 					DeviceAddr;
    ST_INT 					PortSrcAddr;
    ST_UINT64 				PortDstAddr;
    ST_INT 					LocalChannelID;
    ST_INT 					TransmitChannelId;
	ST_INT					DeviceId;
    ST_BOOLEAN 				IsTransfer;
	ST_BOOLEAN 				KnowIPAddr;
protected:
    static const ST_INT		bufLen = 1024;
    ST_BYTE					m_Buffer[bufLen];
    ST_UINT					m_curLength;

};





#endif // PORTTASK_H
