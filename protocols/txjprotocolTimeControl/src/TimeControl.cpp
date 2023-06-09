#include "TimeControl.h"
#include "tinyxml.h"
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/reboot.h>

TimeControl::TimeControl()
{
    //ctor
    m_pLogger       = SysLogger::GetInstance();
    //time(&heat_Newtime);
	//time(&heat_Oldtime);
	firstCheck = false;
}

TimeControl::~TimeControl()
{
    //dtor

}

TimeControl* CreateInstace()
{
    return new TimeControl();
}

ST_VOID	    TimeControl::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{

}
void        TimeControl::Init()
{
    //printf("start load config\r\n");
    Loadconfig();
    if(taskVec.size()>0)
    {
        printf("start thread \r\n");
        m_pLogger->LogDebug("TimeControl start WorkThread !");
        m_WorkTread.Start(WorkTaskProc,this,true);
    }
}

ST_BOOLEAN	TimeControl::OnSend()
{
    if(this->HasTask() && this->GetTask(&m_curTask))
    {
        if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
            //清除任务返回的信息，否则将积压任务体数据在总线上
            Memset(&m_curTask, 0, sizeof(m_curTask));
        }
    }
    return 1;
}

ST_BOOLEAN	TimeControl::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    return 1;
}

ST_BOOLEAN	TimeControl::IsSupportEngine(ST_INT engineType)
{
    return 1;
}


void        TimeControl::Loadconfig()
{
    char *filename = "config/comm_timingDatas.xml";
    TiXmlDocument pReadDocument;
    if(!pReadDocument.LoadFile(filename))
    {
        m_pLogger->LogDebug("Not found config ");
        printf("Not found config\r\n");
    }
    else
    {
        m_pLogger->LogDebug("Load config successful");
        TiXmlElement *pRootElement = pReadDocument.RootElement();
        TiXmlElement *groupList = pRootElement->FirstChildElement("TimingDataGrouplist");
        for (TiXmlElement* groupItem = groupList->FirstChildElement("TimingDataGroup"); groupItem !=0; groupItem = groupItem->NextSiblingElement("TimingDataGroup"))
        {

            struct TaskData tData;
            if(groupItem->FirstChildElement("Type")->GetText())
                tData.type = atoi(groupItem->FirstChildElement("Type")->GetText());

            if(groupItem->FirstChildElement("Times")->FirstChildElement("Count")->GetText())
                tData.m_count = atoi(groupItem->FirstChildElement("Times")->FirstChildElement("Count")->GetText());


            TiXmlElement* timeList = groupItem->FirstChildElement("Times")->FirstChildElement("TimeList");
            if(timeList!=NULL)
            {
                vector<Timeinfo> time_info = getTimeList(timeList);
                tData.timeVec.assign(time_info.begin(),time_info.end());
            }


            TiXmlElement* TDataList = groupItem->FirstChildElement("TaskDatas");
            if(TDataList!=NULL)
            {
                vector<TaskInfo> task_info = getTaskList(TDataList);
                tData.taskVec.assign(task_info.begin(),task_info.end());
            }

            if((tData.timeVec.size()>0)&&(tData.taskVec.size()>0))
            {
                taskVec.push_back(tData);
            }

        }
    }
}




vector<Timeinfo> TimeControl::getTimeList(TiXmlElement* TimeListItem)
{
    vector<Timeinfo> tmpVec;
    for (TiXmlElement* timegroup = TimeListItem->FirstChildElement("TimeGroup"); timegroup !=0; timegroup = timegroup->NextSiblingElement("TimeGroup"))
    {
        TiXmlElement* timeItem = timegroup->FirstChildElement("StartTime");
        if(timeItem!=NULL)
        {
            struct Timeinfo tInfo;
            if(timeItem->FirstChildElement("mesc")->GetText())
                tInfo.mesc = atoi(timeItem->FirstChildElement("mesc")->GetText());

            if(timeItem->FirstChildElement("second")->GetText())
                tInfo.second = atoi(timeItem->FirstChildElement("second")->GetText());

            if(timeItem->FirstChildElement("min")->GetText())
                tInfo.min = atoi(timeItem->FirstChildElement("min")->GetText());

            if(timeItem->FirstChildElement("hour")->GetText())
                tInfo.hour = atoi(timeItem->FirstChildElement("hour")->GetText());

            if(timeItem->FirstChildElement("day")->GetText()){
                int wday = atoi(timeItem->FirstChildElement("day")->GetText());
                tInfo.day = wday>>5;
                //printf("tInfo.day:%d\r\n",tInfo.day);
            }

            if(timeItem->FirstChildElement("month")->GetText())
                tInfo.month = atoi(timeItem->FirstChildElement("month")->GetText());

            if(timeItem->FirstChildElement("year")->GetText())
                tInfo.year = atoi(timeItem->FirstChildElement("year")->GetText());

            printf("day:%d hour :%d min:%d\r\n",tInfo.day,tInfo.hour,tInfo.min);
            tmpVec.push_back(tInfo);
        }

    }

    return tmpVec;
}

vector<TaskInfo> TimeControl::getTaskList(TiXmlElement* TaskListItem)
{
    vector<TaskInfo> tmpVec;
    for (TiXmlElement* TData = TaskListItem->FirstChildElement("TaskData"); TData !=0; TData = TData->NextSiblingElement("TaskData"))
    {
        struct TaskInfo task;

        if(TData->FirstChildElement("ChannelId")->GetText())
            task.channelId = atoi(TData->FirstChildElement("ChannelId")->GetText());

        if(TData->FirstChildElement("DeviceId")->GetText())
            task.DeviceId = atoi(TData->FirstChildElement("DeviceId")->GetText());

        if(TData->FirstChildElement("PointId")->GetText())
            task.PointId = atoi(TData->FirstChildElement("PointId")->GetText());

        if(TData->FirstChildElement("value")->GetText())
            task.value = atoi(TData->FirstChildElement("value")->GetText());

        if(TData->FirstChildElement("LAddr")->GetText())
            task.LAddr = atoi(TData->FirstChildElement("LAddr")->GetText());

        if(TData->FirstChildElement("HAddr")->GetText())
            task.HAddr = atoi(TData->FirstChildElement("HAddr")->GetText());

        if(TData->FirstChildElement("AppendInfo")->GetText())
            task.AppendInfo = TData->FirstChildElement("AppendInfo")->GetText();

        //printf("push back task data!\r\n");
        tmpVec.push_back(task);
    }
    return tmpVec;
}

void        TimeControl::checkTime()
{
    if(!firstCheck)
    {
        //printf("traverse Vec\r\n");
        traverseVec();
 //       time(&heat_Oldtime);
        firstCheck = true;
        return;
    }
    if(checkSec())//1分钟检测一次
    {
        printf("traverse Vec\r\n");
        traverseVec();
    }
}

void        TimeControl::traverseVec()
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep); //取得当地时间
    for(auto iter = taskVec.begin();iter!=taskVec.end();iter++)
    {
        switch((*iter).type){
        case 0:
            {
                int t_hour = (*iter).timeVec[0].hour;
                int t_min  = (*iter).timeVec[0].min;
                char msg[128];
                sprintf(msg,"t_hour:%d ,t_min:%d \r\n p->tm_hour:%d ,p->tm_min:%d p->tm_sec:%d\r\n",t_hour,t_min,(p->tm_hour),(p->tm_min),(p->tm_sec));
                printf(msg);
                //ShowMessage(msg);
                if((t_hour == (p->tm_hour))&& (t_min == (p->tm_min)))
                {
                    printf("tranfer task \r\n");
                    trasnferVec((*iter).taskVec);
                }
            }break;
        case 1:
            {
                int curday = p->tm_wday; /* 一周中的第几天，范围从 0 到 6                */
                if(curday == 0)
                    curday =7;          //当值为0时  是星期日
                char msgs[128];
                sprintf(msgs,"curday:%d \r\n",curday);
                printf(msgs);
                struct Timeinfo TempTime;
                bool res = checkWeekDay(curday,(*iter).timeVec,TempTime);
                if(res)
                {
                    int t_hour = TempTime.hour;//(*iter).timeVec[0].hour;
                    int t_min  = TempTime.min;//(*iter).timeVec[0].min;

                    char msg[128];
                    sprintf(msg,"t_hour:%d ,t_min:%d \r\n p->tm_hour:%d ,p->tm_min:%d p->tm_sec:%d\r\n",t_hour,t_min,(p->tm_hour),(p->tm_min),(p->tm_sec));
                    printf(msg);
                    //ShowMessage(msg);
                    if(t_hour == (p->tm_hour)&& t_min == (p->tm_min))
                    {
                        printf("tranfer task \r\n");
                        ShowMessage("tranfer task ");
                        trasnferVec((*iter).taskVec);
                    }
                }
            }break;
        default:
            break;
    }

    }
}

bool     TimeControl::checkWeekDay(int destDay,vector<Timeinfo> tinfo,struct Timeinfo & tmpTime)
{
    for(int i=0;i<tinfo.size();i++)
    {
        if(destDay == tinfo[i].day)
        {
            //printf("destDay:%d tinfo[i].day:%d\r\n",destDay,tinfo[i].day);
            //printf("tinfo[i].hour:%d,tinfo[i].min:%d\r\n",tinfo[i].hour,tinfo[i].min);
            tmpTime = tinfo[i];
            return true;
        }
    }
    return false;
}

void     TimeControl::trasnferVec(vector<TaskInfo> TVec)
{
    for(int i = 0;i<2;i++)
    {
        int sendNum = 0;
        for(auto iter = TVec.begin();iter!=TVec.end();iter++)
        {
            ProtocolTask task;
            task.isTransfer     = true;
            task.transChannelId = (*iter).channelId;
            task.transDeviceId  = (*iter).DeviceId;
            strcpy(task.taskCmd, "devicecontrol");
            task.taskCmdCode = 1;  //default
            task.taskAddr    = (*iter).LAddr;
            task.taskAddr1    = (*iter).HAddr;
            task.taskValue   = (*iter).value;
            Transfer(&task);
            sleep(0.1);//200ms
            if(++sendNum == 50){
                sleep(3);
                sendNum = 0;
            }
        }
        sleep(2);
    }
}

ST_VOID *TimeControl::WorkTaskProc(ST_VOID *param)
{
    TimeControl *pro = (TimeControl *)param;
    pro->checkTime();
    sleep(1);
	return 0;
}




bool    TimeControl::checkSec()
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep); //取得当地时间
    //printf("p->tm_sec = %d\r\n",p->tm_sec);
    if((p->tm_hour == 0)&&(p->tm_min==0)&&(p->tm_sec==0))
    {
        printf("0 hour 0 min 0 sec reboot dev\r\n");
        sync();
        reboot(RB_AUTOBOOT);
    }
    if(p->tm_sec == 0 )//| p->tm_sec == 1
        return true;
    return false;
}














