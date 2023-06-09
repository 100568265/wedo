#ifndef PORTMQTTCLIENT_H
#define PORTMQTTCLIENT_H
#include "PortBase.h"
#include "MQTTClient.h"
#include "boost/scoped_ptr.hpp"
#include <string>
struct MQTTMSG
{
    int     ACTION;//0:publish 1:subscribe
    char    *Topic;
    int     payloadlen;
    void*   payload;
    int     qos;
};
struct ACTIONTYPE
{
    enum
    {
        PUBLISHMSG = 0x00,
        SUBSCRIBE = 0x01,
        UNSUBSCRIBE = 0x02
    };
};
class PortMQTTClient: public PortBase
{
public:
    PortMQTTClient(Channel *channel, PortInfo *portInfo);
    virtual ~PortMQTTClient();

    ST_VOID					Init  ();
    ST_VOID					Uninit();
    ST_VOID					Open  ();
    ST_VOID					Close ();
    ST_BOOLEAN				Send  (ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size);
    ST_VOID					Recv  ();


    MQTTClient client;
    MQTTClient_connectOptions   connectOpt;
    MQTTClient_message pubMsg;
    MQTTClient_deliveryToken deliveredtoken;


    int ret;    //连接状态
    int rett;   //发送状态
    int connect();
    void disConnect();
    void                    SendPortTask(char *puf,int readlen);
    void connect_lost(void *context, char *cause);
    static std::string clientId;
    static std::string host;
    static std::string user;
    static std::string pwd;
    static std::string pubTopic;
    static std::string subTopic;




protected:
private:



};

#endif // PORTMQTTCLIENT_H
