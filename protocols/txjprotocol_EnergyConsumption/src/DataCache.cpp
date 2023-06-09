#include "DataCache.h"

DataCache::DataCache()
{
    //ctor
}

DataCache::~DataCache()
{
    //dtor

}


struct DataInfo    DataCache::get_front()                //返回第一个元素
{

    struct DataInfo dInfo;
    if(getSize()>0)
    {
        std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
        dInfo = m_queue.front();
    }

    return dInfo;
}

bool                DataCache::pop()                      //删除第一个元素
{
    if(getSize()>0)
    {
        std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
        m_queue.pop();
        return true;
    }

    return false;
}

struct DataInfo     DataCache::get_back()                 //返回最后一个元素
{
    struct DataInfo dInfo;
    if(getSize()>0)
    {
        std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
        dInfo = m_queue.back();
    }
    return dInfo;
}

bool                DataCache::push(DataInfo dInfo)       //在末尾加入一个元素
{
    std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
    if(getSize()>=MAXLINESIZE)
        m_queue.pop();
    m_queue.push(dInfo);
    return true;
}

int                 DataCache::getSize()                  //在末尾加入一个元素
{
    std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
    return m_queue.size();
}

bool                DataCache::isEmpty()                  //如果队列空则返回真
{
    std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
    return m_queue.empty();
}

bool                DataCache::clear_queue()              //清空队列
{
    if(getSize()<1)
        return true;
    std::lock_guard<std::mutex> lk(m_mutex);   //资源上锁 使用lock_guard  避免出现死锁情况
    while (!m_queue.empty()) {
        m_queue.pop();
    }
    return false;
}













