#include <iostream>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <iomanip>
#include "syslogger.h"
#include "rtobjecttree.h"
#include "CmnRtInterface.h"
//#include "HistoryStorage.h"


using namespace std;

SysLogger *m_pLogger;
extern NodeTree *g_pTree;
//HistoryStorage historystorage;

string getCurrentTime(const string& format, const int offset_seconds)
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now + chrono::seconds(offset_seconds));
    tm* now_tm = localtime(&now_c);

//    stringstream ss;
//    ss << put_time(now_tm, format.c_str());
//    return ss.str();
    char ss[80];
    ss[80] << strftime(ss,sizeof(ss),format.c_str(),now_tm);
    return ss;
}



int main()
{

    string filepath = "/opt/";

    //string createtime = getCurrentTime("%Y-%m-%d",0);
    string createtime = getCurrentTime("%d-%H-%M",0);

    string filename = filepath + createtime + ".txt";
    ofstream file(filename); // 创建文件对象并打开文件


    if(file.good()){
            cout<<"file open successfully"<<endl;
        }
        else if(!file.good()){
            cout<<"file open failed"<<endl;
        }



    while (true) {

        //file << getCurrentTime("H%-M%-S%",0)<<endl;

        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm tm = *localtime(&t);
        char oss[80];
        oss[80] << strftime(oss,sizeof(oss),"%H:%M:%S",&tm);



        int minute = tm.tm_min;
        //string inputdata = historystorage.InputData();
        if(minute%2==0);
        {
            //file << oss << " - " << inputdata << endl;//输出到文件中
            file << oss << " - " << endl;//输出到文件中
            sleep(60); // 等待五分钟
        }


        //string newcreatetime = getCurrentTime("%Y-%m-%d",0);
        //string newcreatetime = getCurrentTime("%d-%H-%M",0);
        string newcreatetime = getCurrentTime("%d-%H-%M",0);

        string newfilename = filepath + newcreatetime + ".txt";

        //ofstream newfile;

        if(newcreatetime == createtime)
        {
            continue;
        }

        file.close();
        createtime = newcreatetime;
        file.open(newfilename);

        if(file.good()){
            cout<<"new file open successfully"<<endl;
        }
        else if(!file.good()){
            cout<<"new file open failed"<<endl;
        }




    }
    file.close(); // 关闭文件
    return 0;
}
