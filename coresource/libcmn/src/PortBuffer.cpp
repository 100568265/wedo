//#include "stdafx.h"
#include "PortBuffer.h"

PortBuffer::PortBuffer(void)
{
	m_DataCaches.insert(make_pair((ST_UINT64)-1, new DataCache()));
	m_pTimer=new CTimer();
	m_pTimer->Init(TimeProc,this,50);
	m_pTimer->Start();
}

PortBuffer::~PortBuffer(void)
{
	m_pTimer->Stop();
	map<ST_UINT64,DataCache*>::iterator iter;
	for(iter=m_DataCaches.end();iter!=m_DataCaches.begin();iter--){
        DataCache *cache=(DataCache*)iter->second;
		delete cache;
    }
   m_DataCaches.clear();
   delete m_pTimer;
   m_pTimer=NULL;
}

ST_INT PortBuffer::GetCount()
{
	return m_DataCaches.size();
}

ST_VOID PortBuffer::Write(PortTask *task)
{
//linweiming	m_Mutex.Lock();
	if(IsExists(task->PortSrcAddr)){
		DataCache *pCache=GetCache(task->PortSrcAddr);
		pCache->m_uptTime=time(NULL);
		pCache->WriteBytes(task->GetBuffer(),task->GetBufferLen());
	}
	else{
		DataCache *newCache=new DataCache();
		newCache->m_CacheAddr=task->PortSrcAddr;
		newCache->WriteBytes(task->GetBuffer(),task->GetBufferLen());
		newCache->m_uptTime=time(NULL);
		m_DataCaches.insert(make_pair(newCache->m_CacheAddr,newCache));
	}
//linweiming	m_Mutex.UnLock();
}

ST_BOOLEAN  PortBuffer::IsExists(ST_UINT64 port)
{
	map<ST_UINT64,DataCache*>::iterator m_cli;
    m_cli=m_DataCaches.find(port);
    if(m_cli!=m_DataCaches.end()){
        return true;
	}
	return false;
}

DataCache *PortBuffer::GetCache(ST_UINT64 port)
{
	m_Mutex.TryLock();
	DataCache* cache=NULL;
    map<ST_UINT64,DataCache*>::iterator m_cli;
    m_cli=m_DataCaches.find(port);
    if(m_cli!=m_DataCaches.end()){
        cache=(DataCache*)m_cli->second;
	}
	return cache;
}

ST_VOID PortBuffer::DelCache(ST_UINT64 port)
{
	m_Mutex.TryLock();
	DataCache *cache=GetCache(port);
	if(NULL!=cache){
		m_DataCaches.erase(port);
		delete cache;
	}
}


#ifdef _WIN32
ST_UINT32 CALLBACK PortBuffer::TimeProc(void *param)
#else
ST_VOID *PortBuffer::TimeProc(void *param)
#endif
{
	PortBuffer* pBuffer = (PortBuffer*)param;
	pBuffer->m_curTime  = time(NULL);
	return 0;

	double diff = 0;
	map<ST_UINT64,DataCache*>::iterator iter;
	for(iter = pBuffer->m_DataCaches.end(); iter != pBuffer->m_DataCaches.begin(); iter--) {
		DataCache *cache = (DataCache*)iter->second;
		diff = difftime(cache->m_uptTime, pBuffer->m_curTime);
		if(diff > 600) {
			pBuffer->m_DataCaches.erase(iter);
			delete cache;
		}
    }
	return 0;
}
