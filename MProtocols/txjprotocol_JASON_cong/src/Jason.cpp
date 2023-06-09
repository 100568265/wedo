#include "Jason.h"
#include <unistd.h>
#include "EngineBase.h"
#include <math.h>
#include "Channel.h"
#include "cJSON.h"
#include <stdio.h>

extern NodeTree *g_pTree;

inline void gettime(char *cur_time)
{
    //memset(cur_time,0,sizeof(cur_time));
    memset(cur_time,0,Strlen(cur_time));
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);   //Get time message

    sprintf(cur_time,"%d%02d%02d%02d%02d%02d",t->tm_year+1900
                                                  ,t->tm_mon+1
                                                  ,t->tm_mday
                                                  ,t->tm_hour
                                                ,t->tm_min,t->tm_sec);
}
Jason::Jason()
{
    //ctor
}

Jason::~Jason()
{
    //dtor
}

Jason* CreateInstace()
{
	return new Jason ();
}

void	Jason::Init()
{

}
void	Jason::Uninit()
{

}

void	Jason::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{

}
bool	Jason::OnSend()
{
    getVariableValue();
    return 1;
}
bool	Jason::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    return 1;
}
bool	Jason::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void  Jason::getVariableValue()
{
    cJSON *js_data,*root;
    js_data = cJSON_CreateObject();
    root = cJSON_CreateObject();
    char sTime[64];
    cJSON_AddStringToObject(root,"type","real");
    cJSON_AddStringToObject(root,"sn",this->m_pDevice->GetDeviceInfo()->Addressex);
    gettime(sTime);
    cJSON_AddStringToObject(root,"time",sTime);

    ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
	for(ST_INT i = 0; i < devcount; ++i)
	{
	    cJSON *dev;

		if (g_pTree->GetNodeName("", i, devname) < 0)
        {
            ShowMessage("Not this device");
            //return;
            continue;
        }

        if((strcmp(devname,"channelstate")==0)|(strcmp(devname,"devicestate")==0))
            continue;
		ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        if (varcount <= 0) {
            ShowMessage("varcount is zero");
            return;
        }
        cJSON_AddItemToObject(js_data,devname,dev= cJSON_CreateArray());

		for(int j = 0; j < varcount; ++j)
		{
            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

		    g_pTree->GetNodeName(devname, j, ditname);
            char vardataName[80];
            sprintf(vardataName,"%s.%s",devname,ditname);     //设备名.变量名
            strcpy(fullname, devname);
            strcat(fullname, ".");
		    strcat(fullname, ditname);
		    strcat(fullname, ".value");
		    ST_DUADDR tdd;
		    g_pTree->GetNodeAddr(fullname, tdd);
            ST_VARIANT vValue;
            GetVariableValueByAddr(tdd, vValue);
            switch(vValue.vt)
            {
            case VALType_Float:
                {
                    ST_FLOAT Value  = vValue.fVal;
                    char cValue[48];
                    sprintf(cValue,"%f",Value);

                    cJSON *devDate;
                    cJSON_AddItemToArray(dev,devDate = cJSON_CreateObject());

                    cJSON_AddStringToObject(devDate,"id",ditname);
                    cJSON_AddStringToObject(devDate,"quality","0");
                    cJSON_AddStringToObject(devDate,"value",cValue);
                }
                break;
            default:
                {
                    ShowMessage("not search type");
                }
                break;
            }

       }

    }
    cJSON_AddItemToObject(root,"data",js_data);
    char *printMSG = cJSON_Print(root);  //printMSG 为拼装后的json格式数据
    //this->Send((ST_BYTE*)printMSG,strlen(printMSG));
    m_pLogger->LogInfo("jsonInfo: %s",printMSG);
    SendDagaMsg(printMSG);

}

void        Jason::SendDagaMsg(char *cpayload)
{
    struct MQTTMSG sendMSG;
    string StrTopic = "zg/iotgateway";
//    StrTopic.append(m_id);
    sendMSG.ACTION = ACTIONTYPE::PUBLISHMSG;
    sendMSG.Topic = (char *)StrTopic.c_str();
    sendMSG.payload = (void *)cpayload;
    sendMSG.payloadlen = Strlen(cpayload);
    sendMSG.qos = 1;

    printf("Strlen(cpayload): %d\n",Strlen(cpayload));
    ST_BYTE *sendbuf = new ST_BYTE[sizeof(sendMSG)];
    Memcpy(sendbuf,&sendMSG,sizeof(sendMSG));
    this->Send(sendbuf,sizeof(sendMSG));

    delete[] sendbuf;
}
