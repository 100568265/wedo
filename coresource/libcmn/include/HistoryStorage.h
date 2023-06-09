#ifndef HISTORYSTORAGE_H
#define HISTORYSTORAGE_H

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <cstring>
#include <iomanip>
#include <cstdio>
#include "systhread.h"
#include "syslogger.h"
#include "rtobjecttree.h"
#include "CmnRtInterface.h"
//#include "SaveObject.h"
#include "boost/date_time/gregorian/gregorian.hpp"



using namespace std;

//struct varNode{
//    ST_DUADDR *duaddr;
//    string     nodeName;
//};


class HistoryStorage
{
    public:
        HistoryStorage();
        virtual ~HistoryStorage();

        string getCurrentTime(const string& format, const int offset_seconds);
        ST_VOID Storage();

        //vector<varNode>  vec_Node;

        ST_VOID  Stop();
        ST_VOID Start();

        ST_VOID AddVariable(ST_DUADDR addr,ST_VARIANT value);

    protected:
//        vector<SaveObject>    savelist;

    private:
        ST_BOOLEAN         run = true;
        ST_INT m_filecount = 1;
        Thread	      m_Thread;
        static ST_VOID	*RunStorage(ST_VOID *param);

};

#endif // HISTORYSTORAGE_H
