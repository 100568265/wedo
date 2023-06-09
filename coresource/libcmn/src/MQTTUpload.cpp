#include "MQTTUpload.h"
#include "cJSON.h"
#define wDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote


/*
#define uri "120.25.255.175:1883"
#define clientId "202302"
#define user "rjgf"
#define pwd "rjgf6688"
#define pubTopic "v1/devices/me/telemetry"
*/

extern NodeTree *g_pTree;
extern std::string PortMQTTClient::host;
extern std::string PortMQTTClient::user;
extern std::string PortMQTTClient::pwd;
extern std::string PortMQTTClient::pubTopic;
extern std::string PortMQTTClient::clientId;




int isConnected(MQTTAsync client)
{
    return MQTTAsync_isConnected(client);
}

MQTTUpload::MQTTUpload()
{
    //ctor
}

MQTTUpload::~MQTTUpload()
{
    //dtor
}

ST_VOID *MQTTUpload::RunMQTT(ST_VOID *param)
{
    if (param == nullptr)
    {
        return nullptr;
    }

    MQTTUpload *vs = static_cast<MQTTUpload*>(param);
    vs->Upload();
    //m_pLogger1->GetInstance()->LogInfo("RunMQTT is already running");
    return nullptr;
}

ST_VOID MQTTUpload::Start()
{
    //run = true;
    m_Thread.Start(RunMQTT,this,true);
    m_pLogger1->GetInstance()->LogInfo("Start is already running");
}

ST_VOID MQTTUpload::Stop()
{
    //run = false;
    m_Thread.Stop();
    m_pLogger1->GetInstance()->LogInfo("Stop is already running");
}



void MQTTUpload::Upload()
{
    //MQTTUpload p1;
    init();
    connect();
    while(1)
    {
        getRtVariable();
    }
    disconnect();
}



void MQTTUpload::init()
{
    uri = PortMQTTClient::host;
    clientId = PortMQTTClient::clientId;
    user = PortMQTTClient::user;
    pwd = PortMQTTClient::pwd;
    pubTopic = PortMQTTClient::pubTopic;
    //m_pLogger->GetInstance()->LogInfo("client_id : %s, pub_topic : %s, host: %s, user : %s, passwd : %s",clientId.c_str(), pubTopic.c_str(), host.c_str(), user.c_str(), pwd.c_str());




    // 1、创建一个MQTT客户端
    if ((ret = MQTTClient_create(&client, uri.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("Failed to create client, return code %d\n", ret);
        //exit(EXIT_FAILURE);
        return;
    }
    // 2、创建一个MQTT连接配置结构体，并配置其参数
    connectOpt = MQTTClient_connectOptions_initializer;
    connectOpt.username = user.c_str();    // 用户名
    connectOpt.password = pwd.c_str();    // 用户名对应的密码
    connectOpt.keepAliveInterval = 60; // 心跳时间
    connectOpt.cleansession = 1;       // 清除会话
}

int MQTTUpload::connect()
{
    //3.连接MQTT服务器
    if ((con_ret = MQTTClient_connect(client, &connectOpt)) != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("Failed to connect, return code %d\n", con_ret);
        SysLogger::GetInstance()->LogInfo("uri=%s, clienId = %s, username = %s, password = %s, pubTopic= %s", uri.c_str(),clientId.c_str(),user.c_str(),pwd.c_str(),pubTopic.c_str());
    }
    return con_ret;
}

void MQTTUpload::publish(char *jsonStr)
{
    if(pub_ret!=0 || con_ret!=0)
    {
        this->connect();
    }
    //定义一个主题消息存储结构体
    message = MQTTClient_message_initializer;

    //发布消息的配置
    message.payload = jsonStr;
    message.payloadlen = strlen(jsonStr);
    message.qos = 1;                 // qos等级为1
    message.retained = 0;            // 服务器不保留
    //开始发布数据
    if ((pub_ret = MQTTClient_publishMessage(client, pubTopic.c_str(), &message, &token)) != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("Failed to publish message, return code %d\n", pub_ret);
        return;
    }


}

// 10、断开连接
void MQTTUpload::disconnect()
{
    if ((ret = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("Failed to disconnect, return code %d\n", ret);
        ret = EXIT_FAILURE;
    }
    else
    {
        //SysLogger::GetInstance()->LogError("MQTTClient disconnected.\n");
    }
}



void MQTTUpload::getRtVariable()
{
    Thread::SLEEP(500);
    time_t tt = time(0);
    //产生“YYYY-MM-DD hh:mm:ss”格式的字符串。
    char sTime[32];
    strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", localtime(&tt));
    ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};

    ST_INT devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
    m_pLogger1->GetInstance()->LogInfo("The device count is %d",devcount);



    for(ST_INT i = 0; i < devcount; ++i)
    {
        if (g_pTree->GetNodeName("", i, devname) < 0)
            continue;

        if (strcmp(devname,"devicestate") == 0 || strcmp(devname,"channelstate") == 0)
        {
            continue;
        }

        ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        m_pLogger1->GetInstance()->LogInfo("The varcount is %d",varcount);

        if (varcount <= 0)
            continue;
        cJSON *root = cJSON_CreateArray();
        cJSON *js_body;

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
            //m_pLogger1->GetInstance()->LogInfo("value--------------------------------------%f",vValue.fVal);
            ST_FLOAT Value = vValue.fVal;//获取值
            //m_pLogger1->GetInstance()->LogInfo("Avalue------------------------------------%f",Value);

            js_body = cJSON_CreateObject();
            cJSON_AddStringToObject(js_body,"name",fullname);

            //按数据类型写值
            switch(vValue.vt)
            {
            case VALType_Byte:
            {
                ST_BYTE Value = vValue.bVal;
                cJSON_AddStringToObject(js_body,"type","Boolean");
                //cJSON_AddNumberToObject(js_body,"value",Value);
                std::string strval = Value ? "true" : "false";
                cJSON_AddStringToObject(js_body,"value",strval.c_str());
            }
            break;
            case VALType_Float:
            {
                ST_FLOAT Value  = vValue.fVal;
                cJSON_AddStringToObject(js_body,"type","float");
                char fchar[32];
                sprintf(fchar,"%f",Value);
                cJSON_AddStringToObject(js_body,"value",fchar);
            }
            break;
            case VALType_Double:
            {
                ST_DOUBLE Value  = vValue.dtVal;
                cJSON_AddStringToObject(js_body,"type","double");
                char fchar[32];
                sprintf(fchar,"%lf",Value);
                cJSON_AddStringToObject(js_body,"value",fchar);
            }
            break;
            default:
                break;
            }
            cJSON_AddStringToObject(js_body,"timestamp",sTime);
            cJSON_AddStringToObject(js_body,"qualitystamp","0");
            cJSON_AddItemToObject(root,"",js_body);

            //char *out = cJSON_Print(root);
            /*if(strlen(out) > 1024)
            {
                publish(out);
                cJSON_Delete(root);
                free(out);
                root = cJSON_CreateObject();
            }*/
        }
        char *out = cJSON_Print(root);
        SysLogger::GetInstance()->LogInfo("string : %s",out);
        /*if(strlen(out)<10)
        {
            cJSON_Delete(root);
            free(out);
            return ;
        }*/

        //发布消息
        publish(out);
        cJSON_Delete(root);
        free(out);

    }

}



















