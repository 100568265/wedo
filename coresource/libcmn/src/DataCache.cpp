
#include "DataCache.h"

#include "systhread.h"
#include "GlobalCfg.h"
#include "sysmalloc.h"

DataCache::DataCache():
m_MaxLength(MAXBUFFERLEN),
m_curLength(0)
{
	m_CacheAddr = 0;
    m_Buffer    = new ST_BYTE[m_MaxLength];
	m_uptTime   = time(0);
}

DataCache::DataCache(ST_INT maxLen):
m_curLength(0)
{
    m_MaxLength = (maxLen > 0 ? maxLen : 0);
    m_Buffer    = new ST_BYTE[m_MaxLength];
	m_uptTime   = time(0);
}

DataCache::~DataCache()
{
    if (m_Buffer)
        delete[] m_Buffer;
}

ST_INT DataCache::GetSize()
{
    return m_curLength;
}

ST_INT DataCache::GetLastSize()
{
    return m_MaxLength - m_curLength;
}

ST_BYTE *DataCache::GetBuffer()
{
    return m_Buffer;
}

ST_VOID DataCache::TakeBytes(ST_BYTE *buf, ST_INT size, ST_BOOLEAN erase)
{
	if(buf != NULL) {
		Memcpy(buf, m_Buffer, size);
	}
    if(erase) {
        ST_INT lastSize = m_curLength - size;
        if(lastSize > 0) {
            Memmove (m_Buffer, m_Buffer + size, lastSize);
        }
        m_curLength = lastSize;
    }
    m_Mutex.Notify();
}

ST_INT DataCache::ReadBytes(ST_BYTE *buf, ST_INT size, ST_INT msec)
{
    // if(m_curLength <= 0) return 0;
    if (size > m_MaxLength)
        size = m_MaxLength;

	if(m_curLength < size && msec >= 0)
    {
		// if (m_curLength < size)
		// 	m_Mutex.Wait(msec);
        ST_INT count = msec / 50 + ((msec % 50) ? 1: 0);
        while (count --> 0 && m_curLength < size)
            Thread::SLEEP(50);     
	}
    if (size > m_curLength)
        size = m_curLength;
    this->TakeBytes(buf, size, true);
    return size;
}

ST_INT DataCache::PickBytes(ST_BYTE *buf, ST_INT size, ST_INT msec)
{
    if (size > m_MaxLength)
        size = m_MaxLength;
    Thread::SLEEP(50);
    if(m_curLength < size && msec >= 0)
    {
        // if (m_curLength < size)
        //     m_Mutex.Wait(msec);
        ST_INT count = msec / 50 + ((msec % 50) ? 1: 0);
        while (count --> 0 && m_curLength < size)
            Thread::SLEEP(50);
    }
    if (size > m_curLength)
        size = m_curLength;
    this->TakeBytes(buf, size, false);
    return size;
}

ST_BOOLEAN DataCache::WriteBytes(ST_BYTE *buf, ST_INT size)
{
    while(size > (m_MaxLength - m_curLength)) {
        m_Mutex.Wait(0);
    }
    if(size > 0) {
        if(size > m_MaxLength) {
            ST_BYTE * tempbuf = new ST_BYTE[size];
            Memcpy(tempbuf, buf, size);
            ST_BYTE * swapbuf = m_Buffer;
            m_Buffer = tempbuf;
            m_MaxLength = m_curLength = size;
            if (swapbuf) delete[] swapbuf;
        }
        else {
            if(size > (m_MaxLength - m_curLength)) {
                Memcpy(m_Buffer, buf, size);
                m_curLength = size;
            }
            else {
                Memcpy(m_Buffer + m_curLength,buf,size);
                m_curLength += size;
            }
        }
        m_Mutex.Notify();
    }
    return true;
}

ST_VOID DataCache::Clear()
{
    m_curLength = 0;
    m_Mutex.Notify();
}
