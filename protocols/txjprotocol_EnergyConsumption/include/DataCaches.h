#ifndef DATACACHES_H
#define DATACACHES_H
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

class DataCaches
{
    public:
        DataCaches();
        virtual ~DataCaches();

        struct DataInfo     get_front();                //返回第一个元素
        struct DataInfo     get_back();                 //返回最后一个元素
        bool                push(DataInfo dInfo);       //在末尾加入一个元素
        int                 getSize();                  //在末尾加入一个元素
        bool                isEmpty();                  //如果队列空则返回真
        bool                clear_queue();              //清空队列

        virtual bool        pop();                      //删除第一个元素

    protected:

    private:
        std::queue<DataInfo>    m_queue;
        std::mutex              m_mutex;
};

#endif // DATACACHES_H
