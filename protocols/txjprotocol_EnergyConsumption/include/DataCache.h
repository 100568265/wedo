#ifndef DATACACHE_H
#define DATACACHE_H
#include "datatype.h"
#include <queue>
#include <mutex>
#define MAXLINESIZE 1024

struct DataInfo{
        union{
            ST_BYTE *pBuf;
            ST_CHAR *cBuf;
        };
        int pSize;
        DataInfo()
        {
            pBuf = NULL;
            pSize = 0;
        }
};
class DataCache
{
    public:

        DataCache();
        virtual ~DataCache();
        struct DataInfo     get_front();                //返回第一个元素
        struct DataInfo     get_back();                 //返回最后一个元素
        bool                push(DataInfo dInfo);       //在末尾加入一个元素
        bool                pop();                      //删除第一个元素
        int                 getSize();                  //在末尾加入一个元素
        bool                isEmpty();                  //如果队列空则返回真
        bool                clear_queue();              //清空队列

    protected:

    private:
        std::queue<DataInfo> m_queue;
        std::mutex m_mutex;


};
/*
    back()返回最后一个元素

    empty()如果队列空则返回真

    front()返回第一个元素

    pop()删除第一个元素

    push()在末尾加入一个元素

    size()返回队列中元素的个数
*/
#endif // DATACACHE_H
