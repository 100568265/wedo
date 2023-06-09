#ifndef DATACACHE_H
#define DATACACHE_H

#include <stddef.h>
#include "datatype.h"
#include "sysmutex.h"


#ifdef _WIN32
class __declspec(dllexport) Mutex;
class COMMUNICATIONDLL_API DataCache
#else
class DataCache
#endif
{
public:
    DataCache();
    DataCache(ST_INT maxLen);
    virtual ~DataCache();
    ST_INT				GetSize();
    ST_INT				GetLastSize();
    ST_BYTE				*GetBuffer();
    ST_INT				ReadBytes (ST_BYTE * buf, ST_INT size, ST_INT msec = 100);
    ST_INT				PickBytes (ST_BYTE * buf, ST_INT size, ST_INT msec = 100);
    ST_BOOLEAN			WriteBytes(ST_BYTE * buf, ST_INT size);
    ST_VOID				Clear();
private:
    ST_VOID				TakeBytes(ST_BYTE *buf, ST_INT size, ST_BOOLEAN erase);
public:
	ST_UINT64			m_CacheAddr;
	time_t				m_uptTime;

    ST_BYTE				*m_Buffer;
    Mutex				m_Mutex;
private:
    ST_INT              m_MaxLength;
    ST_INT              m_curLength;
};

#endif // BUFFER_H
