#ifndef SYSQUEQUE_H
#define SYSQUEQUE_H

#include "datatype.h"

template<class T>
class Queue
{
    public:
        struct QNode
        {
            T* data;
            struct QNode *next;
        };
    public:
        Queue();
        virtual ~Queue();
        T*        Pop();
        T*        Peek();
        ST_INT	  GetCount();
        ST_VOID Push(T *data);
        ST_BOOLEAN IsEmpty();
    private:
        ST_INT m_count;
        QNode *m_pHead;
};

template<class T> Queue<T>::Queue()
{
    m_count=0;
    m_pHead = NULL;
}

template<class T> Queue<T>::~Queue()
{
    QNode *pNode = m_pHead;
    QNode *pLast = m_pHead;
    while (pNode != NULL){
        pLast = pNode;
        pNode = pNode->next;
        delete pLast;
    }
    m_count = 0;
}

template<class T> T* Queue<T>::Pop()
{
    if(NULL==m_pHead){
        m_count=0;
        return NULL;
    }
    else{
        QNode *pTmp=m_pHead;
        m_pHead=m_pHead->next;
        m_count--;
        return pTmp->data;
    }
}

template<class T> T* Queue<T>::Peek()
{
    if(NULL==m_pHead){
        m_count=0;
        return NULL;
    }
    else{
        QNode *pTmp=m_pHead;
        return pTmp->data;
    }
}

template<class T> ST_BOOLEAN Queue<T>::IsEmpty()
{
    return m_count==0;
}

template<class T> ST_VOID Queue<T>::Push(T *data)
{
    QNode *pNode=new QNode();
	pNode->next = NULL;
	pNode->data=data;
	if(NULL==m_pHead ){
	    m_count=0;
            m_pHead = pNode;
	}
	else{
		QNode *pTmp = m_pHead;
		while(pTmp->next){
			pTmp = pTmp->next;
		}
		pTmp->next = pNode;
	}
	m_count++;
}

template<class T> ST_INT Queue<T>::GetCount()
{
	return m_count;
}

#endif // SYSQUEQUE_H
