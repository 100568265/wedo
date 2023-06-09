#include "CKJJSON.h"
#include "cJSON.h"
#include "syslogger.h"
#include "Communication.h"
#include "ChannelConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <list>
#include "Debug.h"
#include "rtbase.h"

#define wDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote

extern NodeTree *g_pTree;

char *random_uuid( char buf[37] )
{
    const char *c = "89ab";
    char *p = buf;
    int n;
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;
        switch( n )
        {
        case 6:
            sprintf(p, "4%x", b%15 );
            break;
        case 8:
            sprintf(p, "%c%x", c[rand()%strlen(c)], b%15 );
            break;
        default:
            sprintf(p, "%02x", b);
            break;
        }

        p += 2;
        switch( n )
        {
        case 3:
        case 5:
        case 7:
        case 9:
            *p++ = '-';
            break;
        }
    }
    *p = 0;
    return buf;
}


CKJJSON::CKJJSON()
{
    //ctor
    is_auth = false;
    is_firstLogin = false;

}

CKJJSON::~CKJJSON()
{
    //dtor
}

CKJJSON* CreateInstace()
{
    return new CKJJSON();
}

void	    CKJJSON::Init()
{

    char cuuid[37] = {0};
    m_token = random_uuid(cuuid);

    /*
    time_t tt = time(0);
    //产生“YYYY-MM-DD hh:mm:ss”格式的字符串。
    char sTime[32];
    strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", localtime(&tt));
    m_startTime = sTime;
    */

    PortInfo *pinfo = this->GetMainPort()->GetPortInfo();
    m_id = pinfo->user;
    m_password = pinfo->passwd;
    m_pubTopic = pinfo->pub_topic;
}
void	    CKJJSON::Uninit()
{


}

void	    CKJJSON::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
	if(this->GetCurPort())
	{
	    ST_INT	len = this->GetCurPort()->PickBytes(pbuf, 1024, 300);
		if (len >= 0)
		{
		    int ndatalen = this->GetCurPort()->ReadBytes(pbuf, len);
            readed = ndatalen;
		}
	}

}
ST_BOOLEAN  CKJJSON::OnSend()
{
   if(!is_firstLogin){
        if(this->IsOpened()){
            LoginAuth();
            is_firstLogin = true;
            is_auth = true; // for_test
            InitTransfertable();
            return true;
        }
    }


    if(is_auth){ //
        {
            SendVecData();
			return true;
		}
    }
    return true;
}

bool	    CKJJSON::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    /*
    ShowMessage((char *)pbuf);
    cJSON *json,*js_id,*js_result,*js_token,*js_type;
    string sid,sresult,stoken,stype;
    json = cJSON_Parse((const char *)pbuf);
    js_id  = cJSON_GetObjectItem(json,"id");
    js_result =  cJSON_GetObjectItem(json,"result");
    js_token = cJSON_GetObjectItem(json,"token");
    js_type = cJSON_GetObjectItem(json,"type");

    sid = js_id->valuestring;
    sresult = js_result->valuestring;
    stoken = js_token->valuestring;
    stype = js_type->valuestring;


    if(!sresult.compare("Succeed")){
        m_pLogger->GetInstance()->LogInfo("Auth Succeed!");
        is_auth = true;
    } //Authorized
     else if(!sresult.compare("Authorized")){
        m_pLogger->GetInstance()->LogInfo("MQTT Client Authorized!");
        is_auth = true;
    }
    else {
        m_pLogger->GetInstance()->LogInfo("MQTT Client auth error : %s!",sresult.c_str());
    }
    */

    return true;
}

bool	    CKJJSON::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

bool CKJJSON::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
	if (!this->GetDevice())
		return false;
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables) {
		printf(" No transfer tables.\n");
		return false;
	}

	if (trantables->GetCount() <= 0) {
		printf("Transfer table count is 0.\n");
		return false;
	}
	int iter = 0;
	for (; iter < trantables->GetCount(); ++iter)
	{
		if (index == trantables->GetItem(iter)->typeId())
			break;
	}
	if ((table = trantables->GetItem(iter)) == NULL) {
		printf("Not have this transfer table.index:%d\n",index);
		return false;
	}
	else
        printf("Get this transfer table.index :%d\n", index);

	if ((list = table->m_pVarAddrs) == NULL) {
		printf("Not have this list.\n");
		return false;
	}
	return true;
}





void        CKJJSON::SendVecData()
{
    if(vec_Node.size()==0)
    {
        ShowMessage("transfer table is null");
        return ;
    }

    time_t tt = time(0);
		//产生“YYYY-MM-DD hh:mm:ss”格式的字符串。
    char sTime[32];
    strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", localtime(&tt));

    cJSON *root = cJSON_CreateArray();
    cJSON *js_body;

    //ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0},valuename[256] = {0};
    //Strcpy(devname,m_devName.c_str());
    for(unsigned int i = 0; i < vec_Node.size(); ++i)
    {

        ST_VARIANT vValue;
        GetVariableValueByAddr(*vec_Node[i].duaddr, vValue);

        if(vec_Node[i].duaddr->type == -2) continue;
        if(vec_Node[i].duaddr->type == -1) continue;

        js_body = cJSON_CreateObject();

        //cJSON_AddStringToObject(js_body,"name",vec_Node[i].nodeName.c_str());

        switch(vValue.vt)
        {
        case VALType_Byte:
        {
                        /*
            ST_BYTE Value = vValue.bVal;
            cJSON_AddStringToObject(js_body,"type","Boolean");
            string strval = Value ? "true" : "false";
            cJSON_AddStringToObject(js_body,"value",strval.c_str()); */
        }
        break;
        case VALType_Float:
        {
            ST_FLOAT Value  = vValue.fVal;
            //cJSON_AddStringToObject(js_body,"type","float");
            char fchar[32];
            sprintf(fchar,"%f",Value);

             cJSON_AddStringToObject(js_body,  vec_Node[i].nodeName.c_str() ,fchar);
        }
        break;
        case VALType_Double:
        {
            ST_DOUBLE Value  = vValue.dtVal;
            //cJSON_AddStringToObject(js_body,"type","double");
            char fchar[32];
            sprintf(fchar,"%lf",Value);
            cJSON_AddStringToObject(js_body,vec_Node[i].nodeName.c_str(),fchar);
        }
        break;
        default:
            break;
        }
        //cJSON_AddStringToObject(js_body,"timestamp",sTime);
        //cJSON_AddStringToObject(js_body,"qualitystamp","0");
        cJSON_AddItemToObject(root,"",js_body);

        char *out = cJSON_Print(root);
        if(Strlen(out) > 1024)
        {
            SendDagaMsg(out);
            cJSON_Delete(root);
            free(out);
            root = cJSON_CreateArray();
        }

    }
    char *out = cJSON_Print(root);
    if(Strlen(out)<10)
    {
        cJSON_Delete(root);
        free(out);
        return ;
    }

    SendDagaMsg(out);
    cJSON_Delete(root);
    free(out);
}

void        CKJJSON::SendDagaMsg(char *cpayload)
{
    struct MQTTMSG sendMSG;
    string StrTopic = m_pubTopic;
    StrTopic.append(m_id);
    sendMSG.ACTION = ACTIONTYPE::PUBLISHMSG;
    sendMSG.Topic = (char *)StrTopic.c_str();
    sendMSG.payload = (void *)cpayload;
    sendMSG.payloadlen = Strlen(cpayload);
    sendMSG.qos = 1;

    ST_BYTE *sendbuf = new ST_BYTE[sizeof(sendMSG)];
    Memcpy(sendbuf,&sendMSG,sizeof(sendMSG));
    this->Send(sendbuf,sizeof(sendMSG));

    delete[] sendbuf;
}

void        CKJJSON::LoginAuth()
{
    struct MQTTMSG sendMSG;
    // string StrTopic = "v1/device/auth";
    // string StrTopic = "v1/devices/me/telemetry";
    // char *topic = (char *)StrTopic.c_str();
    char *topic = (char *)m_pubTopic.c_str();

    cJSON * root;
    root=cJSON_CreateObject();   //创建根数据对象
    cJSON_AddStringToObject(root,"id",m_id.c_str());  //加入键值，加字符串
    cJSON_AddStringToObject(root,"password",m_password.c_str());
    // cJSON_AddStringToObject(root,"token",m_token.c_str());
    // cJSON_AddStringToObject(root,"os","arm linux");
    // cJSON_AddStringToObject(root,"startTime",m_startTime.c_str());
    // cJSON_AddStringToObject(root,"appVersion","1.0");
    // cJSON_AddStringToObject(root,"type","add");
    char *out = cJSON_Print(root);
    ShowMessage(out);

    sendMSG.ACTION = ACTIONTYPE::PUBLISHMSG;
    sendMSG.Topic = topic;
    sendMSG.payload = (void *)out;
    sendMSG.payloadlen = Strlen(out);
    sendMSG.qos = 1;

    ST_BYTE *sendbuf = new ST_BYTE[sizeof(sendMSG)];
    Memcpy(sendbuf,&sendMSG,sizeof(sendMSG));
    this->Send(sendbuf,sizeof(sendMSG));


    cJSON_Delete(root);
    free(out);

    delete[] sendbuf;
}



void        CKJJSON::InitTransfertable()
{
    printf("Enter Init transfer\n");

    for (int index=0; index< 3; index++) {
        TransferTable *trantable   = NULL;
        List<ST_DUADDR> *tablelist = NULL;

        if (!CheckTransferTableExist (index, trantable, tablelist))
            continue;

        int32_t list_size = tablelist->GetCount();
        if (list_size <= 0) {
            list_size = 0;
        }

        for (int32_t iter = 0; iter < list_size; ++iter)
        {
            ST_DUADDR  *duaddr = tablelist->GetItem(iter);
            if (duaddr != NULL)
            {
                Device *t_dev = this->m_pChannel->GetCommunication()->GetChannel(duaddr->connect)->GetDevice(duaddr->device);
                string s_devName = t_dev->GetDeviceInfo()->DeviceName;
                printf("get device name:%s\n", s_devName.c_str());

                ST_CHAR  ditName[256] = {0};
                // GetDitNameByAddr(t_dev->GetDeviceInfo(),duaddr->addr,ditName);
                if (Rt_GetNodeNameByAddr(duaddr, ditName) >= 0)
                {
                    ST_CHAR fullName[512] = {0};
                    replace(s_devName.begin(),s_devName.end(),'-','.');
                    strcat(fullName, ditName);

                    printf("get ditname:%s \n", fullName);

                    struct varNode var;

                    var.duaddr = duaddr;
                    var.nodeName = fullName;
                    var.nodeName = var.nodeName.substr(0, var.nodeName.length() - 6);


                    vec_Node.push_back(var);
                }
            }
            else
                printf("get duaddr fault.\n");
        }
    }
    printf("End Init Transfer\n");
}

void        CKJJSON::GetDitNameByAddr(DeviceInfo *devinfo,int id,ST_CHAR* ditName)
{
    int areaCount = devinfo->DataAreasCount;
    for(int i=0;i<areaCount;i++){
        int itemsize = devinfo->DataAreas[i].itemCount;
        printf("%s area count:%d index:%d itemCOunt: %d; cur id:%d \n", devinfo->DeviceName, areaCount, i, itemsize, id);
        for(int j=0;i<itemsize;j++){
            int addr = devinfo->DataAreas[i].items[j].id;
            if(addr==id){
                Strcpy(ditName,devinfo->DataAreas[i].items[j].itemName);
                printf("find item name:%s", devinfo->DataAreas[i].items[j].itemName);
                return ;
            }
        }
    }
}

//















