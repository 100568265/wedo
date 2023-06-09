#ifndef PORTBUFFER_H
#define PORTBUFFER_H

#include <map>
#include "DataCache.h"
#include "PortTask.h"
#include "Timer.h"
using namespace std;

class PortBuffer
{
public:
	PortBuffer(void);
	~PortBuffer(void);
	ST_INT						GetCount();
	ST_VOID                     DelCache(ST_UINT64 port);
    ST_VOID						Write(PortTask *task);
	DataCache                   *GetCache(ST_UINT64 port);
protected:
	ST_BOOLEAN                  IsExists(ST_UINT64 port);
#ifdef _WIN32
	static ST_UINT32 CALLBACK	TimeProc(void *param);
#else
	static ST_VOID				*TimeProc(void *param);
#endif
protected:
	map<ST_UINT64,DataCache*>	m_DataCaches;
	Mutex						m_Mutex;
	CTimer						*m_pTimer;
	time_t						m_curTime;
};

#endif //PORTBUFFER_H
