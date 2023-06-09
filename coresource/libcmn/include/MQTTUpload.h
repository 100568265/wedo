#ifndef CODE_PORTMQTTCLIENT_H
#define CODE_PORTMQTTCLIENT_H
#include "MQTTClient.h"
#include "PortMQTTClient.h"
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <cstring>
#include <iomanip>
#include <cstdio>
#include "systhread.h"
#include "syslogger.h"
#include "rtobjecttree.h"
#include "CmnRtInterface.h"
#include "MQTTAsync.h"

class MQTTUpload
{
public:
    MQTTUpload();
        virtual ~MQTTUpload();
    MQTTClient client;
    MQTTClient_connectOptions connectOpt;
    MQTTClient_message message;
    MQTTClient_deliveryToken token; // 标记MQTT消息的值，用来检查消息是否发送PortMQTTClient(Channel *channel, PortInfo *portInfo);成功

    void init();    //初始化MQTT客户端的各项参数
    int connect();  //连接服务器
    void publish(char *jsonStr);  //发布消息
    void disconnect();  //断开连接
    void getRtVariable();
    ST_VOID  Stop();
    ST_VOID Start();

private:
    std::string clientId;
    std::string uri;
    std::string user;
    std::string pwd;
    std::string pubTopic;
    std::string subTopic;
    static ST_VOID *RunMQTT(ST_VOID *param);
    void Upload();
    Thread m_Thread;
    SysLogger *m_pLogger1;
    int con_ret;
    int pub_ret;
    int ret;

};


#endif //CODE_PORTMQTTCLIENT_H
