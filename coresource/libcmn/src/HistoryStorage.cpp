#include "HistoryStorage.h"
//#include "Debug.h"
#define wDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote

HistoryStorage::HistoryStorage()
{
    //ctor
}

HistoryStorage::~HistoryStorage()
{
    //dtor
}

SysLogger *m_pLogger;
extern NodeTree *g_pTree;

string HistoryStorage::getCurrentTime(const string& format, const int offset_seconds)
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

ST_BOOLEAN GetValue(const ST_VARIANT& var,ST_DOUBLE &preValue)
{
    preValue = 0;
    switch(var.vt)
    {
    case VALType_Byte:
        preValue = var.bVal;
        break;
    case VALType_Int16:
        preValue = var.sVal;
        break;
    case VALType_Int32:
        preValue = var.iVal;
        break;
    case VALType_Int64:
        preValue = var.lVal;
        break;
    case VALType_UInt16:
        preValue = var.usVal;
        break;
    case VALType_UInt32:
        preValue = var.uiVal;
        break;
    case VALType_UInt64:
        preValue = var.ulVal;
        break;
    case VALType_Float:
        preValue = var.fVal;
        break;
    case VALType_Boolean:
        preValue = var.blVal ;
        break;
    case VALType_Double:
        preValue = var.dtVal ;
        break;
    case VALType_Decimal:
        return SD_FAILURE;
    case VALType_DateTime:
        return SD_FAILURE;
    case VALType_String:
        return SD_FAILURE;
    case VALType_Binary:
        return SD_FAILURE;
    default:
        return SD_FAILURE;
    }
    return SD_SUCCESS;
}



ST_VOID *HistoryStorage::RunStorage(ST_VOID *param)
{
    if (param == nullptr)
    {
        return nullptr;
    }
    //HistoryStorage *vs = (HistoryStorage*)param;

    HistoryStorage *vs = static_cast<HistoryStorage*>(param);
    vs->Storage();
    m_pLogger->GetInstance()->LogInfo("RunStorage is already runing");
    return nullptr;
}

ST_VOID HistoryStorage::Start()
{
    //run = true;
    m_Thread.Start(RunStorage,this,true);
    m_pLogger->GetInstance()->LogInfo("Start is already runing");
}

ST_VOID HistoryStorage::Stop()
{
    //run = false;
    m_Thread.Stop();
    m_pLogger->GetInstance()->LogInfo("Stop is already runing");
}

void HistoryStorage::Storage()
{
    m_pLogger->GetInstance()->LogInfo("Storage is already runing");
    string filepath = "/opt/";

    //string createtime = getCurrentTime("%Y-%m-%d",0);
    string createtime = getCurrentTime("%d-%H-%M",0);

    string filename = filepath + createtime + ".txt";
    ofstream file(filename); // 创建文件对象并打开文件


    if(file.good())
    {
        m_pLogger->GetInstance()->LogInfo("The file open successfully");
        //cout<<"file open successfully"<<endl;
    }
    else if(!file.good())
    {
        m_pLogger->GetInstance()->LogInfo("The file open failed");
        //cout<<"file open failed"<<endl;
    }



    while (true)
    {

        Thread::SLEEP(500);

        //file << getCurrentTime("H%-M%-S%",0)<<endl;

        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm tm = *localtime(&t);
        char oss[80];
        oss[80] << strftime(oss,sizeof(oss),"%H:%M:%S",&tm);



        int minute = tm.tm_min;
        int second = tm.tm_sec;

        //string inputdata = historystorage.InputData();
        if((second % 5) == 0)
        {
            //file << oss << " - " << inputdata << endl;//输出到文件中
            file << oss << endl;//输出时间到文件中

            ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};

            ST_INT  devcount = g_pTree->GetNameNodeCount(NULL)-5;  //获取设备数量
            m_pLogger->GetInstance()->LogInfo("The device count is %d",devcount);



            for(ST_INT i = 0; i < devcount; ++i)
            {
                if (g_pTree->GetNodeName("", i, devname) < 0)
                    continue;

                if (strcmp(devname,"devicestate") == 0 || strcmp(devname,"channelstate") == 0)
                {
                    continue;
                }

                ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
                m_pLogger->GetInstance()->LogInfo("The varcount is %d",varcount);
                vector<vector<string>> StorageData(devcount, vector<string>(varcount, ""));

                if (varcount <= 0)
                    continue;

                for(ST_INT j = 0; j < varcount; ++j)
                {
                    *(int32_t*)fullname = 0;
                    *(int32_t*)ditname = 0;
                    //memset(fullname,0,sizeof(fullname)/2);
                    //memset(ditname,0,sizeof(ditname)/2);

                    g_pTree->GetNodeName(devname, j, ditname);

                    strcpy(fullname, devname);

                    strcat(fullname, ".");
                    strcat(fullname, ditname);
                    strcat(fullname, ".value");

                    ST_DUADDR tdd;
                    ST_VARIANT vValue;
                    g_pTree->GetNodeAddr(fullname, tdd);
                    GetVariableValueByAddr(tdd, vValue);
                    m_pLogger->GetInstance()->LogInfo("value--------------------------------------%f",vValue.fVal);
                    ST_FLOAT Value = vValue.fVal;//获取值
                    m_pLogger->GetInstance()->LogInfo("Avalue------------------------------------%f",Value);
                    string StorageValue = to_string(Value);//值转字符串
                    StorageData[i][j] = StorageValue;
                    file << fullname << "\"" << StorageData[i][j] << "\"" <<endl;
                }

            }
            sleep(3); // 等待4秒
        }




////////////////////////////////////////////////////////////////////////////////////////////////测试版删除七分钟前的文件
        char oldest_file_name[20];
        if(minute>7)
        {
            sprintf(oldest_file_name,"%s%02d-%02d-%02d.txt","/opt/",tm.tm_mday,tm.tm_hour,tm.tm_min-7);
            int deleteresult = remove(oldest_file_name);
            if(deleteresult == 0)
            {
                m_pLogger->GetInstance()->LogInfo("delete oldest_file_name successfully");
            }
            if(deleteresult == -1)
            {
                m_pLogger->GetInstance()->LogInfo("delete oldest_file_name failed");
            }
        }
        if(minute<=7)
        {
            sprintf(oldest_file_name,"%s%02d-%02d-%02d.txt","/opt/",tm.tm_mday-1,tm.tm_hour+23,tm.tm_min+53);
            int deleteresult = remove(oldest_file_name);
            if(deleteresult == 0)
            {
                m_pLogger->GetInstance()->LogInfo("delete oldest_file_name successfully");
            }
            if(deleteresult == -1)
            {
                m_pLogger->GetInstance()->LogInfo("delete oldest_file_name failed");
            }
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
    }

    file.close(); // 关闭文件

}
