#ifndef TIMECONTROL_H
#define TIMECONTROL_H
#include "Protocol.h"
#include <vector>
#include "syslogger.h"
#include "tinyxml.h"
#include "systhread.h"
#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;
struct Timeinfo{
    int mesc;
    int second;
    int min;
    int hour;
    int day;
    int month;
    int year;

    Timeinfo& operator=(Timeinfo& tf)
    {
        mesc = tf.mesc;
        second = tf.second;
        min = tf.min;
        day = tf.day;
        hour = tf.hour;
        month = tf.month;
        year = tf.year;
    }
};

struct TaskInfo{
    int channelId;
    int DeviceId;
    int PointId;
    int value;
    int LAddr;
    int HAddr;
    string AppendInfo;
    TaskInfo& operator=(TaskInfo& tf)
    {
        channelId = tf.channelId;
        DeviceId = tf.DeviceId;
        PointId = tf.PointId;
        value = tf.value;
        LAddr = tf.LAddr;
        HAddr = tf.HAddr;
        AppendInfo = tf.AppendInfo;
    }
};

struct TaskData{
    int type;
    int m_count;
    vector<Timeinfo> timeVec;
    vector<TaskInfo> taskVec;

    TaskData& operator=(TaskData& tf)
    {
        timeVec.assign(tf.timeVec.begin(),tf.timeVec.end());
        taskVec.assign(tf.taskVec.begin(),tf.taskVec.end());
        m_count = tf.m_count;
        type = tf.type;
    }
};

class TimeControl: public Protocol
{
    public:
        TimeControl();
        virtual ~TimeControl();
        void        Init();
        ST_VOID	    OnRead(ST_BYTE* pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

        void        checkTime();
    protected:
        static ST_VOID				*WorkTaskProc(ST_VOID *param);
    private:
        vector<TaskData>    taskVec;
        SysLogger	        *m_pLogger;
        void                Loadconfig();
        void                traverseVec();                      //遍历时间集合
        void                trasnferVec(vector<TaskInfo> TVec); //转发任务
        bool                checkWeekDay(int destDay,vector<Timeinfo> tinfo,struct Timeinfo & tmpTime);
        bool                checkSec();
        vector<Timeinfo>    getTimeList(TiXmlElement* TimeListItem);
        vector<TaskInfo>    getTaskList(TiXmlElement* TaskListItem);


        //time_t 		    heat_Newtime,heat_Oldtime;
        Thread			m_WorkTread;
        bool            firstCheck;

        ProtocolTask    m_curTask;

};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	TimeControl* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // TIMECONTROL_H
