#include "CIEC101.h"
#include "sysmutex.h"
#include "iec60870_5_101_types.h"
#include "iec60870_5_101_obj.h"
#include "datetime.h"
#include "Debug.h"
#include "FakeTimer.h"
#include <cmath>
#include <queue>
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "CIni.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
*/
volatile time_t FakeTimer::curr_sec;

extern NodeTree *g_pTree;
int s_exit_flag = 0;
static int s_show_headers = 0;
const char *s_show_headers_opt = "--show-headers";
#define logWarn		SysLogger::GetInstance()->LogWarn
#define sDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote

char handleMsg[1000];
clock_t t1=clock();
clock_t t2=clock();
// extern NodeTree *g_pTree;
CIEC101* CreateInstace()
{
	return new CIEC101 ();
}


/*  获取INI文件数据
    * 函数名：         GetIniKeyString
    * 入口参数：        title
    *                      配置文件中一组数据的标识
    *                  key
    *                      这组数据中要读出的值的标识
    *                  filename
    *                      要读取的文件路径
    * 返回值：         找到需要查的值则返回正确结果 0
    *                  否则返回-1
    */
inline int GetIniKeyString(char *title,char *key,char *filename,char *buf)
{
    FILE *fp;
    int  flag = 0;
    char sTitle[64], *wTmp;
    char sLine[1024];
    sprintf(sTitle, "[%s]", title);

    if(NULL == (fp = fopen(filename, "r"))) {
        perror("fopen");
        return -1;
    }
    while (NULL != fgets(sLine, 1024, fp)) {
        // 这是注释行
        if (0 == strncmp("//", sLine, 2)) continue;
        if ('#' == sLine[0])              continue;
        wTmp = strchr(sLine, '=');
        if ((NULL != wTmp) && (1 == flag)) {
            if (0 == strncmp(key, sLine, strlen(key))) { // 长度依文件读取的为准
                sLine[strlen(sLine) - 1] = '\0';
                fclose(fp);
                while(*(wTmp + 1) == ' '){
                    wTmp++;
                }
                strcpy(buf,wTmp + 1);
                return 0;
            }
        } else {
            if (0 == strncmp(sTitle, sLine, strlen(sTitle))) { // 长度依文件读取的为准
                flag = 1; // 找到标题位置
            }
        }
    }
    fclose(fp);
    return -1;
}

static bool readConfigFile(const char * cfgfilepath, const string & key, string & value)
{
    fstream cfgFile;
    cfgFile.open(cfgfilepath);//打开文件
    if( ! cfgFile.is_open())
    {
        cout<<"can not open cfg file!"<<endl;
        return false;
    }
    char tmp[1000];
    while(!cfgFile.eof())//循环读取每一行
    {
        cfgFile.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
        string line(tmp);
        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
        if(pos==string::npos) return false;
        string tmpKey = line.substr(0,pos);//取=号之前
        if(key==tmpKey)
        {
            value = line.substr(pos+1);//取=号之后
            return true;
        }
    }
    return false;
}

inline int DectoHex(int dec, unsigned char *hex, int length)
{
	int i;
	for (i = length - 1; i >= 0; i--)
	{
		hex[i] = (dec % 256) & 0xFF;
		dec /= 256;
	}
	return 0;
}

CIEC101::CIEC101()
{
	//ctor

}

CIEC101::~CIEC101()
{
	//dtor
//	delete m_obj;
}

void CIEC101::Init ()
{

}

void CIEC101::Uninit()
{

}

void CIEC101::OnRead (ST_BYTE* pbuf, ST_INT& readed)
{

}

bool CIEC101::OnSend ()
{
    SendJsonData2Post();
    t1 = clock();
    if((t1-t2)/CLOCKS_PER_SEC >= 10)
    {
        t2 = clock();
        sendHeatBeat();
    }
	return true;
}

inline bool operator== (const CtrlField& lhs, const CtrlField& rhs)
{
	return *((uint8_t*)&lhs) == *((uint8_t*)&rhs);
}

bool CIEC101::OnProcess (ST_BYTE* pbuf, ST_INT len)
{

}

bool CIEC101::IsSupportEngine (ST_INT engineType)
{
	return 0 == engineType;
}



inline void gettime(char *cur_time)file:///home/work/wedo/comm/protocols/txjprotocolicbciot%20(%E5%89%AF%E6%9C%AC)

{
    memset(cur_time,0,sizeof(cur_time));
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);   //Get time message

    sprintf(cur_time,"%d-%02d-%02d %02d:%02d:%02d",t->tm_year+1900
                                                  ,t->tm_mon+1
                                                  ,t->tm_mday
                                                  ,t->tm_hour
                                                ,t->tm_min,t->tm_sec);
}
static void http_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_CONNECT:
      if (*(int *) ev_data != 0) {/*
        fprintf(stderr, "connect() failed: %s\n", strerror(*(int *) ev_data));*/
        sprintf(handleMsg,"connect() failed: %s",strerror(*(int *) ev_data));
        //strcpy(handleMsg,stderr);
//        sprintf()
        s_exit_flag = 1;
      }else{
       strcpy(handleMsg,"Server  connection");
      }
      break;
    case MG_EV_HTTP_REPLY:
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        strcat(handleMsg,hm->body.p);
        s_exit_flag = 1;

      break;
    case MG_EV_CLOSE:
      if (s_exit_flag == 0) {
//        printf("Server closed connection\n");
        strcpy(handleMsg,"Server closed connection");
        s_exit_flag = 1;
      }
      break;
    case MG_EV_TIMER:
      s_exit_flag=1;
      break;
    default:
      break;
  }
}
//#define sDebug	if (false) wedoDebug (SysLogger::GetInstance()).noquote

 ST_VOID * CIEC101::sendHttpPost(ST_VOID *obj){

    char header[1024];
    Strcpy(header,"charset: utf-8\r\nContent-Type: application/json\r\n");

    char *parm=(char *) obj ; //json参数

/*    char url[1024];
    GetIniKeyString("data","url","UUID.ini",url);*/
    string url;
    readConfigFile("config.cfg","url",url);

//    "http://47.111.14.175:8088/CloudIot/MesRoot/DeviceIot/Upload"
 //   GetIniKeyString("data","encode","UUID.ini",encode);
/*    sprintf(url,"http://192.168.1.46:8081");
        //url="http://192.168.1.46:8081";*/
//    int i;
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);
    mg_connect_http(&mgr,http_handler, url.c_str() ,header, parm);
    free(parm);
    while (s_exit_flag == 0) {
        mg_mgr_poll(&mgr, 3000);
    }
    mg_mgr_free(&mgr);
    return 0;
 // return ;//获取配置文件中的uuid
}
/*
void CIEC101::writeData2File(char *devName,char *jsonData)
{
    char fileName[128];
    sprintf(fileName,"dev/%s.json",devName);
    int fId = open(fileName,O_CREAT|O_WRONLY,strlen(jsonData));  //以覆盖写的形式打开，O_CREAT：无次文件则创建
    if(fId !=-1)
    {
        flock(fId, LOCK_EX);
 //       ShowMessage("open file success");
        write(fId,jsonData,strlen(jsonData));
        close(fId);
        flock(fId, LOCK_UN);
    }
    else
    {
        ShowMessage("open file error");
    }

}
*/

void  CIEC101::SendJsonData2Post()
{
    getVariableValue();
}

void  CIEC101::getVariableValue()
{
    ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
	for(ST_INT i = 0; i < devcount; ++i)
	{
        cJSON *js_data,*root;
        js_data = cJSON_CreateObject();
        root = cJSON_CreateObject();
/*
        char js_uuid[128];
        GetIniKeyString("data","UUID","UUID.ini",js_uuid);  //获取配置文件中的uuid*/

        string uuid;
        readConfigFile("config.cfg","uuid",uuid);
//        ShowMessage(uuid.c_str());

//        cJSON_AddStringToObject(root,"UUID",uuid.c_str());

        cJSON_AddStringToObject(root,"UUID","e4e4ce1c-0e6c-4225-91e7-e1d6312906e3");

		if (g_pTree->GetNodeName("", i, devname) < 0)
        {
            ShowMessage("Not this device");
            return;
        }
        if(strcmp(devname,"channelstate")==0|strcmp(devname,"devicestate")==0)
            continue;
		ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        if (varcount <= 0) {
            ShowMessage("varcount is zero");
            return;
        }
/*
		for(int j = 0; j < varcount; ++j)
		{
            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

		    g_pTree->GetNodeName(devname, j, ditname);
            char vardataName[80];
            sprintf(vardataName,"%s.%s",devname,ditname);     //设备名.变量名
            strcpy(fullname, devname); strcat(fullname, ".");
		    strcat(fullname, ditname); strcat(fullname, ".value");
		    ST_DUADDR tdd;
		    g_pTree->GetNodeAddr(fullname, tdd);
            ST_VARIANT vValue;
            char typeValue[60];
            GetVariableValueByAddr(tdd, vValue);
            int typeNum = vValue.vt;
            sprintf(typeValue,"typeNum:%d",typeNum);
            switch(vValue.vt)
            {
            case VALType_SByte:
                {
                   ST_BYTE Value = vValue.bVal;
                   char cValue[48];
                   sprintf(cValue,"%d",Value);
 //                  cJSON_AddNumberToObject(js_data,vardataName,Value);
                   cJSON_AddStringToObject(js_data,vardataName,cValue);
                }
                break;
            case VALType_Byte:
                {
                    ST_BYTE Value = vValue.bVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Boolean:
                {
                    ST_BYTE Value = vValue.blVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Int16:
                {
                    ST_INT Value  = vValue.sVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_UInt16:
                {
                    ST_INT Value  = vValue.usVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Int32:
                {
                    ST_INT Value  = vValue.iVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_UInt32:
                {
                    ST_INT Value  = vValue.uiVal;
                    char cValue[48];
                    sprintf(cValue,"%d",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
                   //cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Float:
                {
                    ST_FLOAT Value  = vValue.fVal;
                    char cValue[48];
                    sprintf(cValue,"%f",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Int64:
                {
                    ST_LONG Value  = vValue.lVal;
                    char cValue[48];
                    sprintf(cValue,"%f",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
//                    cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_UInt64:
                {
                    ST_LONG Value  = vValue.ulVal;
                    char cValue[48];
                    sprintf(cValue,"%f",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
 //                   cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            case VALType_Double:
                {
                    ST_DOUBLE Value  = vValue.dtVal;
                    char cValue[225];
                    sprintf(cValue,"%f",Value);
                    cJSON_AddStringToObject(js_data,vardataName,cValue);
//                    cJSON_AddNumberToObject(js_data,vardataName,Value);
                }
                break;
            default:
                {
                    ShowMessage("not search type");
                }
                break;
            }

       }*/

       cJSON_AddStringToObject(js_data,"tmChargeTime","10.54");
       cJSON_AddStringToObject(js_data,"tmClpClsTime","5.00");
       cJSON_AddStringToObject(js_data,"tmClpOpnPosi","391.20");
       cJSON_AddStringToObject(js_data,"tmClpOpnTime","2.13");

        cJSON_AddItemToObject(root,"data",js_data);
        char timebuf[48];
        gettime(timebuf);
        cJSON_AddStringToObject(root,"datetime",timebuf);
        char *printMSG = cJSON_Print(root);  //printMSG 为拼装后的json格式数据
//        ShowMessage(printMSG);
        m_thread.Start(sendHttpPost,printMSG,false);
        ShowMessage(handleMsg);

        string url;
        readConfigFile("config.cfg","url",url);
        ShowMessage(url.c_str());
        ShowMessage("http://47.111.14.175:8088/CloudIot/MesRoot/DeviceIot/Upload");
        if(url.c_str()=="http://47.111.14.175:8088/CloudIot/MesRoot/DeviceIot/Upload")
        {
            ShowMessage("The address is the same");
        }
        else
        {
            ShowMessage("Address different");
        }
        //sendHttpPost(printMSG);
        /*
        char url[1024];
        GetIniKeyString("data","url","UUID.ini",url);*/
//        char *url = GetIniKeyString2("data","url","UUID.ini");
//        ShowMessage(url);
    }


}

//2019-08-22 10:43:25
//2019-05-06 15:50:54
void CIEC101::sendHeatBeat()
{
    cJSON *root;
    root = cJSON_CreateObject();
    char js_uuid[128];
    GetIniKeyString("data","UUID","UUID.ini",js_uuid);  //获取配置文件中的uuid
    cJSON_AddStringToObject(root,"UUID",js_uuid);

    char timebuf[48];
    gettime(timebuf);
    cJSON_AddStringToObject(root,"datetime",timebuf);
    char *printMSG = cJSON_Print(root);  //printMSG 为拼装后的json格式数据
    cJSON_Delete(root);
 //   sendHttpPost(printMSG);
/*
    char KeepAlive[128];
    GetIniKeyString("url","KeepAlive","UUID.ini",KeepAlive);  //获取配置文件中的uuid
    ShowMessage(KeepAlive);*/


}


