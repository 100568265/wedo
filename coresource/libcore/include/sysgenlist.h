#ifndef SYSGENLIST_H
#define SYSGENLIST_H

#include "datatype.h"
#include "sysmalloc.h"

template <class T>
class List
{
private:
	T** m_pItems;
	ST_INT	m_maxCount;
	ST_INT	m_curCount;

    ST_VOID Init()
    {
        m_pItems = (T**)Malloc(sizeof(T*)*m_maxCount);
        Memset(m_pItems, 0, sizeof(T*)*m_maxCount);
        m_curCount = 0;
    }

    ST_VOID CheckFree()
    {
        if(m_curCount >= (m_maxCount - 1))
        {
            T** tmp = m_pItems;
            m_maxCount = 2*m_maxCount;
            m_pItems = (T**)Malloc(sizeof(T*)*m_maxCount);
            Memset(m_pItems,0,sizeof(T*)*m_maxCount);
            Memcpy(m_pItems,tmp,sizeof(T*)*m_curCount);
            Free(tmp);
        }
    }
public:
    List(ST_VOID)
    {
        m_maxCount = 10;
        Init();
    }

    List(ST_INT max)
    {
        m_maxCount = (max <= 0 ? 10 : max);
/*        if(m_maxCount <= 0)
        {
            m_maxCount = 10;
        }*/
        Init();
    }

    ~List(ST_VOID)
    {
        Clear();
    }

    ST_INT Add(T* item)
    {
        CheckFree();
        m_pItems[m_curCount++] = item;
        return m_curCount - 1;
    }

    ST_VOID Clear()
    {
        ST_INT tmpCount = m_curCount;
        m_curCount = 0;
        for(ST_INT i = 0; i < tmpCount; i++)
        {
            delete m_pItems[i];
            m_pItems[i] = NULL;
        }
    }

    inline ST_INT GetCount()
    {
        return m_curCount;
    }

    T* GetItem(ST_INT index)
    {
        if(index >= m_curCount) return NULL;
        return m_pItems[index];
    }

    ST_BOOLEAN Remove(T* item)
    {
        ST_INT index = this->IndexOf(item);
        if(index != -1)
        {
            RemoveAt(index);
            return SD_TRUE;
        }
        return SD_FALSE;
    }

    ST_VOID RemoveAt(ST_INT index)
    {
        if(index < 0 || index >= this->m_curCount)return;
        ST_INT movecount = m_curCount - index - 1;
        if(movecount > 0)
        {
            Memcpy(m_pItems + index, m_pItems + index + 1, movecount*sizeof(T*));
        }
        --m_curCount;
    }

    ST_VOID Insert(ST_INT index, T* item)
    {
        if(index < 0 || index >= this->m_curCount)index = this->m_curCount - 1;
        ST_INT movecount = m_curCount - index - 1;
        if(movecount > 0)
        {
            CheckFree();
            Memcpy(m_pItems + index + 1, m_pItems + index, movecount*sizeof(T*));
        }
        else
        {
            m_pItems[m_curCount] == item;
        }
        ++m_curCount;
    }

    ST_INT IndexOf(T* item)
    {
        for(ST_INT i = 0; i < m_curCount; i++)
        {
            if(m_pItems[i] == item)
                return i;
        }
        return -1;
    }
};

#endif // SYSGENLIST_H
