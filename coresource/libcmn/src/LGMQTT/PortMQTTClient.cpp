#include "PortMQTTClient.h"
#include "syslogger.h"
#include "Channel.h"
//#include "MQTT/confighandler.h"
//#include "MQTT/sha1.hpp"
//#include "LGMQTT/HMAC_MD5_API.h"
#include <time.h>
#include <stdio.h>
#include <exception>





// 失去连接回调函数
void PortMQTTClient::connect_lost(void *context, char *cause)
{
    printf("Connection lost,The reason: %s \n", cause);
}


// 收到主题信息回调函数
int message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Receive topic: %s, message data: \n", topicName);
    printf("%.*s\n", message->payloadlen, (char *)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

// 主题发布成功回调函数
void delivery_complete(void *context, MQTTClient_deliveryToken dt)
{
    printf("publish topic success,token = %d \n", dt);
}


PortMQTTClient::PortMQTTClient(Channel *channel, PortInfo *portInfo):
    PortBase(channel, portInfo)
{
    //ctor
}

PortMQTTClient::~PortMQTTClient()
{
    //dtor
}

ST_VOID		PortMQTTClient::Init()
{
    if(m_Inited)
        return;
    char str[256] = {0};
    snprintf(str, sizeof(str) - 1, "%s:%d", m_pPortInfo->RemoteAddress, m_pPortInfo->RemotePort);
    host = str;
    clientId = m_pPortInfo->client_id;
    user = m_pPortInfo->user;
    pwd = m_pPortInfo->passwd;
    pubTopic = m_pPortInfo->pub_topic;
    m_pLogger->GetInstance()->LogInfo("client_id : %s, pub_topic : %s, host: %s, user : %s, passwd : %s",
                       clientId.c_str(), pubTopic.c_str(), host.c_str(), user.c_str(), pwd.c_str());
}


ST_VOID		PortMQTTClient::Uninit()
{
    //return 0;
    if(!m_Inited)
        return;
    m_Inited = false;
}

int         PortMQTTClient::connect()
{
    //MQTTClient client;
    ret = MQTTClient_create(&client, host.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    //ret = MQTTClient_create(&client, MQTT_URI, ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (ret != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("MQTTClient_create error");
        return ret;
    }

    connectOpt = MQTTClient_connectOptions_initializer;
    connectOpt.username = user.c_str();
    connectOpt.password = pwd.c_str();
    connectOpt.keepAliveInterval =60;
    connectOpt.cleansession = 1;
    // 4、设置MQTT连接时的回调函数
    //MQTTClient_setCallbacks(client, client, connect_lost, NULL, NULL);

    //TODO:回调函数，有待优化
    // 5、开始连接到MQTT服务器
    if ((ret = MQTTClient_connect(client, &connectOpt)) != MQTTCLIENT_SUCCESS)
    {
        SysLogger::GetInstance()->LogError("MQTT Client connect error. code : (%d)", ret);
        SysLogger::GetInstance()->LogError("uri=%s, clienId = %s, username = %s, password = %s", host.c_str(),clientId.c_str(),connectOpt.username,connectOpt.password);
    }

    return ret;
}

void PortMQTTClient::disConnect()
{
    MQTTClient_disconnect(client,10000);
    //MQTTClient_destroy(&client);
}


ST_VOID		PortMQTTClient::Open()
{
    Locker locker(&(this->m_Mutex));
//   if (m_IsOpened) Close();

    //  	return ;

    ret = connect();
    if (ret != MQTTCLIENT_SUCCESS)
    {
        Thread::SLEEP(1000);
        return;
    }
    else
        m_IsOpened = true;
}
ST_VOID		PortMQTTClient::Close ()
{
    Locker locker(&(this->m_Mutex));
    if (!m_IsOpened)
        return ;
    disConnect();
    m_IsOpened = false;
}



ST_BOOLEAN	PortMQTTClient::Send  (ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size)
{
   if(rett!=0)
   {
       //上次发送失败，原因可能是断开连接
    ret = MQTTClient_connect(client, &connectOpt);
    m_pLogger->GetInstance()->LogDebug("断线重新连接");

   }
   if(!m_IsOpened)
        Open();
    while (!m_IsOpened)
        Thread::SLEEP(100);

    Locker locker(&(this->m_Mutex));
    pubMsg = MQTTClient_message_initializer;

    char* str = (char*)pBuf;
    pubMsg.payload = str; // message
    pubMsg.payloadlen = strlen(str);
    pubMsg.qos = 1;
    pubMsg.retained = 0;      // If we want to get message arrived time, set here to 1
    //m_pLogger->GetInstance()->LogInfo("payload = %s",str);

    rett = MQTTClient_publishMessage(client, pubTopic.c_str(), &pubMsg, &deliveredtoken);
    if (rett != MQTTCLIENT_SUCCESS)
    {
        m_pLogger->GetInstance()->LogError("MQTTClient PublishMessage Error. code : %d", rett);
        return false;
    }

    Thread::SLEEP(100);

    return true;
}


ST_VOID		PortMQTTClient::Recv  ()
{
    /*if (State() != Connected) {
        Thread::SLEEP (1000);
        return;
    }
    Thread::SLEEP (1000);
    MQTTClient_message *receive_msg=NULL;
    char *topic = NULL;
    char  *ptr = NULL;
    int topic_len;
      int rv= MQTTClient_receive(client,&topic,&topic_len,&receive_msg,100000);
        if(rv == MQTTCLIENT_SUCCESS)
        {
            ptr = (char *)receive_msg->payload;
            m_portTask.Write((ST_BYTE *)ptr, receive_msg->payloadlen);
            m_portTask.PortDstAddr = 0; //inet_addr(m_pPortInfo->RemoteAddress);
            m_portTask.LocalChannelID = m_pChannel->GetLocalChannelID();
            m_pChannel->GetCEngine()->ReadTask(&m_portTask);
        }*/

}
