#include "Jason.h"
#include <unistd.h>
#include "EngineBase.h"
#include <math.h>
#include "Channel.h"
#include "cJSON.h"

extern NodeTree *g_pTree;

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
    ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
//	for(ST_INT i = 0; i < devcount; ++i)
	{
        cJSON *js_data,*root,*dev,*devDate1,*devDate2;
        js_data = cJSON_CreateObject();
        root = cJSON_CreateObject();
        devDate1 = cJSON_CreateObject();
        devDate2 = cJSON_CreateObject();
        //dev = cJSON_CreateObject();
//        ShowMessage(uuid.c_str());

//        cJSON_AddStringToObject(root,"UUID",uuid.c_str());

        cJSON_AddStringToObject(root,"type","real");
        cJSON_AddStringToObject(root,"sn","90613005");
        cJSON_AddStringToObject(root,"time","20191225152334");


        cJSON_AddItemToObject(js_data,"XB02-NB01",dev= cJSON_CreateArray());

        cJSON_AddItemToArray(dev,devDate1 );
        cJSON_AddItemToArray(dev,devDate2 );

        cJSON_AddStringToObject(devDate1,"id","dc_input_voltage_1");
        cJSON_AddStringToObject(devDate1,"quality","0");
        cJSON_AddStringToObject(devDate1,"value","509.300");

        cJSON_AddStringToObject(devDate2,"id","dc_input_current_1");
        cJSON_AddStringToObject(devDate2,"quality","0");
        cJSON_AddStringToObject(devDate2,"value","1.350");


        cJSON_AddItemToObject(root,"data",js_data);

 /*
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

        //cJSON_AddItemToObject(root,"data",js_data);
        //char timebuf[48];
        //gettime(timebuf);
        //cJSON_AddStringToObject(root,"datetime",timebuf);
        char *printMSG = cJSON_Print(root);  //printMSG 为拼装后的json格式数据
        ShowMessage(printMSG);

    }


}
