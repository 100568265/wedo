#include "CMqtt.h"
#include "Dispatcher.h"
#include <string.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "syslogger.h"
#include "Communication.h"
#include "ChannelConfig.h"
#include <list>
#include "aiot_dm_api.h"
#include "cJSON.h"
#include "dm_private.h"
#include "core_mqtt.h"
/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static pthread_t g_mqtt_process_thread;
static pthread_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

extern NodeTree *g_pTree;

aiot_subdev_dev_t   g_subdev[300];
int                 addIndex = 0;
bool                m_isSubConn = false;

using namespace std;
CMqtt::CMqtt():
m_Channels(NULL)
{
    //ctor
    m_isConn = -1;
    m_isSubConn = false;
    ykSendDm = false;
    firstSend =false;
    time(&ykNewTime);
	time(&ykOldTime);
	time(&Newcurtime);
	time(&oldcurtime);
}

CMqtt::~CMqtt()
{

    //dtor

}

void	CMqtt::Init()
{
    Init_Channels();
    Init_Protocols();

    Init_MQTT_Conn();
    Init_Subdev();
}

void	CMqtt::Uninit()
{

}

CMqtt* CreateInstace()
{
   return new  CMqtt();
}

void set_subConn_state(int bv)
{
    m_isSubConn = bv;
}

vector<string> split(const string &str, const string &pattern)
{
    vector<string> res;
    if(str == "")
        return res;
    //在字符串末尾也加入分隔符，方便截取最后一段
    string strs = str + pattern;
    size_t pos = strs.find(pattern);

    while(pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        res.push_back(temp);
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos+1, strs.size());
        pos = strs.find(pattern);
    }

    return res;
}

/* 日志回调函数, SDK的日志会从这里输出 */
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    core_mqtt_handle_t *mqhandle;
    mqhandle = (core_mqtt_handle_t *)mqhandle;
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\n");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */


        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\n");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
           SysLogger::GetInstance()->LogWarn ("AIOT_MQTTEVT_RECONNECT");
 /*            CMqtt *cmp = (CMqtt *)mqhandle->protocol;
            cmp->*/
            set_subConn_state(false);
            SysLogger::GetInstance()->LogWarn ("set_subConn_state FALSE");
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? (char *)("network disconnect") :
                          (char *)("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            printf("heartbeat response\n");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        }
        break;

        default: {

        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void *demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        sleep(1);
    }
    return NULL;
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            sleep(1);
        }
    }
    return NULL;
}

/* 用户数据接收处理回调函数 */
static void demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    printf("demo_dm_recv_handler, type = %d\r\n", recv->type);

    switch (recv->type) {

        /* 属性上报, 事件上报, 获取期望属性值或者删除期望属性值的应答 */
        case AIOT_DMRECV_GENERIC_REPLY: {
            printf("msg_id = %d, code = %d, data = %.*s, message = %.*s\r\n",
                   recv->data.generic_reply.msg_id,
                   recv->data.generic_reply.code,
                   recv->data.generic_reply.data_len,
                   recv->data.generic_reply.data,
                   recv->data.generic_reply.message_len,
                   recv->data.generic_reply.message);
        }
        break;

        /* 属性设置 */
        case AIOT_DMRECV_PROPERTY_SET: {
            printf("msg_id = %ld, params = %.*s\r\n",
                   (unsigned long)recv->data.property_set.msg_id,
                   recv->data.property_set.params_len,
                   recv->data.property_set.params);

            /* TODO: 以下代码演示如何对来自云平台的属性设置指令进行应答, 用户可取消注释查看演示效果 */
            /*
            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_PROPERTY_SET_REPLY;
                msg.data.property_set_reply.msg_id = recv->data.property_set.msg_id;
                msg.data.property_set_reply.code = 200;
                msg.data.property_set_reply.data = "{}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }
            */
        }
        break;

        /* 异步服务调用 */
        case AIOT_DMRECV_ASYNC_SERVICE_INVOKE: {
            printf("msg_id = %ld, service_id = %s, params = %.*s\r\n",
                   (unsigned long)recv->data.async_service_invoke.msg_id,
                   recv->data.async_service_invoke.service_id,
                   recv->data.async_service_invoke.params_len,
                   recv->data.async_service_invoke.params);
            printf("recv.product_key:%s,recv.device_name:%s\r\n",recv->product_key,recv->device_name);

            /* TODO: 以下代码演示如何对来自云平台的异步服务调用进行应答, 用户可取消注释查看演示效果
             *
             * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
             */
            cJSON *json,*json_value;
            json = cJSON_Parse(recv->data.async_service_invoke.params);
            json_value = cJSON_GetObjectItem( json , "codeValue" );
            int cValue = json_value->valueint;


            dm_handle_t *handle;
            handle = (dm_handle_t *)dm_handle;
            CMqtt *cmp = (CMqtt *)handle->protocol;
            int dId = cmp->get_devId(recv->device_name);
            cmp->tranfer_YK_task(dId,cValue);

            cmp->ykSendDm = true;
            cmp->ykDevName = recv->device_name;
            time(&(cmp->ykNewTime));
            time(&(cmp->ykOldTime));

            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
                msg.data.async_service_reply.msg_id = recv->data.async_service_invoke.msg_id;
                msg.data.async_service_reply.code = 200;
                msg.data.async_service_reply.service_id = recv->data.async_service_invoke.service_id;
                msg.data.async_service_reply.data = "{\"resCode\": 1}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }

        }
        break;

        /* 同步服务调用 */
        case AIOT_DMRECV_SYNC_SERVICE_INVOKE: {
            printf("msg_id = %ld, rrpc_id = %s, service_id = %s, params = %.*s\r\n",
                   (unsigned long)recv->data.sync_service_invoke.msg_id,
                   recv->data.sync_service_invoke.rrpc_id,
                   recv->data.sync_service_invoke.service_id,
                   recv->data.sync_service_invoke.params_len,
                   recv->data.sync_service_invoke.params);

            /* TODO: 以下代码演示如何对来自云平台的同步服务调用进行应答, 用户可取消注释查看演示效果
             *
             * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id和rrpc_id字符串, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
             */


            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_SYNC_SERVICE_REPLY;
                msg.data.sync_service_reply.rrpc_id = recv->data.sync_service_invoke.rrpc_id;
                msg.data.sync_service_reply.msg_id = recv->data.sync_service_invoke.msg_id;
                msg.data.sync_service_reply.code = 200;
                msg.data.sync_service_reply.service_id = "SetLightSwitchTimer";
                msg.data.sync_service_reply.data = "{}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }

        }
        break;

        /* 下行二进制数据 */
        case AIOT_DMRECV_RAW_DATA: {
            printf("raw data len = %d\r\n", recv->data.raw_data.data_len);
            /* TODO: 以下代码演示如何发送二进制格式数据, 若使用需要有相应的数据透传脚本部署在云端 */
            /*
            {
                aiot_dm_msg_t msg;
                uint8_t raw_data[] = {0x01, 0x02};

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_RAW_DATA;
                msg.data.raw_data.data = raw_data;
                msg.data.raw_data.data_len = sizeof(raw_data);
                aiot_dm_send(dm_handle, &msg);
            }
            */
        }
        break;

        /* 二进制格式的同步服务调用, 比单纯的二进制数据消息多了个rrpc_id */
        case AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE: {
            printf("raw sync service rrpc_id = %s, data_len = %d\r\n",
                   recv->data.raw_service_invoke.rrpc_id,
                   recv->data.raw_service_invoke.data_len);
        }
        break;

        default:
            break;
    }
}

int32_t demo_mqtt_start(void **handle,void **dmhandle,vector<string> msgvec,void *protocol)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    void       *dm_handle = NULL;
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* TODO: 替换为自己设备的三元组
    {
      "ProductKey": "a1WHlPlp2nQ",
      "DeviceName": "SmartGateway1",
      "DeviceSecret": "7b7898a710a5595569478d34b2aeb9df"
    }

    char *product_key       = "a1qmdMV8LLD";//(char *)msgvec[0].c_str();//"a1V6hv8BEUi";
    char *device_name       = "SmartGateway161";//(char *)msgvec[1].c_str();//"test01";
    char *device_secret     = "1036b95f0065302fc93e80314ac6a4ef";//(char *)msgvec[2].c_str();
    */
    char *product_key       = (char *)msgvec[0].c_str();
    char *device_name       = (char *)msgvec[1].c_str();
    char *device_secret     = (char *)msgvec[2].c_str();

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\n");
        return -1;
    }

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }


    snprintf(host, 100, "%s.%s", product_key, url);
    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);
    /* 配置协议指针 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PROTOCOL, (void *)protocol);
     /* 创建DATA-MODEL实例 */
    dm_handle = aiot_dm_init();
    if (dm_handle == NULL) {
        printf("aiot_dm_init failed");
        return -1;
    }
    /* 配置MQTT实例句柄 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    /* 配置消息接收处理回调函数 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)demo_dm_recv_handler);
    /* 配置协议指针 */
    aiot_dm_setopt(dm_handle,AIOT_DMOPT_PROTOCOL,(void *)protocol);
    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\n", -res);
        return -1;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\n", res);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\n", res);
         g_mqtt_process_thread_running = 0;
        pthread_join(g_mqtt_process_thread, NULL);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    *handle = mqtt_handle;
    *dmhandle = dm_handle;
    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    pthread_join(g_mqtt_process_thread, NULL);
    pthread_join(g_mqtt_recv_thread, NULL);

    /* 断开MQTT连接 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    /* 销毁MQTT实例 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

void demo_subdev_recv_handler(void *handle, const aiot_subdev_recv_t *packet, void *user_data)
{
    switch (packet->type) {
        case AIOT_SUBDEVRECV_TOPO_ADD_REPLY:
        case AIOT_SUBDEVRECV_TOPO_DELETE_REPLY:
        case AIOT_SUBDEVRECV_TOPO_GET_REPLY:
        case AIOT_SUBDEVRECV_BATCH_LOGIN_REPLY:
        case AIOT_SUBDEVRECV_BATCH_LOGOUT_REPLY:
        case AIOT_SUBDEVRECV_SUB_REGISTER_REPLY:
        case AIOT_SUBDEVRECV_PRODUCT_REGISTER_REPLY: {
            printf("msgid        : %d\n", packet->data.generic_reply.msg_id);
            printf("code         : %d\n", packet->data.generic_reply.code);
            printf("product key  : %s\n", packet->data.generic_reply.product_key);
            printf("device name  : %s\n", packet->data.generic_reply.device_name);
            printf("message      : %s\n", (packet->data.generic_reply.message == NULL)?("NULL"):(packet->data.generic_reply.message));
            printf("data         : %s\n", packet->data.generic_reply.data);
        }
        break;
        case AIOT_SUBDEVRECV_TOPO_CHANGE_NOTIFY: {
            printf("msgid        : %d\n", packet->data.generic_notify.msg_id);
            printf("product key  : %s\n", packet->data.generic_notify.product_key);
            printf("device name  : %s\n", packet->data.generic_notify.device_name);
            printf("params       : %s\n", packet->data.generic_notify.params);
        }
        break;
        default: {

        }
    }
}

void	    CMqtt::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{

}

ST_BOOLEAN	CMqtt::OnSend()
{
//    if(m_isConn == -1)
//        Init_MQTT_Conn();
//    Send_dm_data();
//    sleep(6);
    return 1;
}

ST_BOOLEAN	CMqtt::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    return 1;
}

ST_BOOLEAN	CMqtt::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void        CMqtt::Init_Channels()
{
    printf("Init_Channels()\r\n");
//    Channel *m_pChannel=NULL;
	m_Channels=new List<Channel>();
    m_Channels=this->m_pChannel->GetCommunication()->GetChannels();

    int count = this->m_pChannel->GetCommunication()->GetChannels()->GetCount();
    for(int i=0;i<count;i++)
    {
        if(this->m_pChannel->GetCommunication()->GetChannels()->GetItem(i)->GetDevices()!=NULL)
        {
            printf("channle name is :%s\r\n",this->m_pChannel->GetCommunication()->GetChannels()->GetItem(i)->GetChannelInfo()->ChannelName);
        }
    }
}

void        CMqtt::Init_Protocols()
{
    printf("Init_Protocols()\r\n");
    for(int i=0;i<m_Channels->GetCount();i++)
    {
        t_channel  = m_Channels->GetItem(i);
        if(t_channel == NULL)
        {
           printf("NULL pointer1\r\n");
           continue;
        }
        string cName(t_channel->GetChannelInfo()->ChannelName);
        if(cName == "state" || cName == "mqtt")
            continue;

        //printf("protocols count : %d\r\n",t_channel->GetCEngine()->m_pProtocols.GetCount());

        t_Protocols = t_channel->GetCEngine()->m_pProtocols;
        if(t_Protocols.GetCount()<1)
        {
           printf("NULL pointer2\r\n");
           continue;
        }
//        printf("protocols count : %d\r\n",t_channel->GetCEngine()->m_pProtocols.GetCount());

        for(int j = 0;j < t_Protocols.GetCount(); j++)
        {
            ProtocolBase  *tp  = *t_Protocols.GetItem(j);
            if(tp != NULL)
            {
                if((tp->GetDevice()->GetDeviceInfo()->Addressex)==NULL)
                    return ;

                string addrex(tp->GetDevice()->GetDeviceInfo()->Addressex);
                string devname(tp->GetDevice()->GetDeviceInfo()->DeviceName);
                AddSubModle(addrex,devname,tp->GetDevice()->GetDeviceInfo()->DeviceId);
//                printf("addrex:%s\r\n",addrex.c_str());
            }

        }
        //t_Protocols.Clear();
    }
}

void        CMqtt::AddSubModle(string addresex,string devname,int deviceId)
{
//    printf("AddSubModle()\r\n");
    vector<string> msgvec;
    msgvec = split(addresex,"|");
    if(msgvec.size()<4)
        return;

    const DeviceInfo & minfo = *this->GetDevice()->GetDeviceInfo();
    string info_secret(minfo.Addressex);
    vector<string> posvec;
    posvec = split(info_secret,"|");


    subdev_modle t_modle ;
    memset(t_modle.devname,0,sizeof(t_modle.devname));
    memcpy(t_modle.devname,devname.c_str(),strlen(devname.c_str()));

    t_modle.deviceId = deviceId;

    t_modle.subMsg.product_key = new char[strlen(msgvec[1].c_str())+1];
    strcpy(t_modle.subMsg.product_key,msgvec[1].c_str());



    t_modle.subMsg.device_name = new char[strlen(msgvec[2].c_str())+1];
    strcpy(t_modle.subMsg.device_name,msgvec[2].c_str());



    t_modle.subMsg.device_secret = new char[strlen(msgvec[3].c_str())+1];
    strcpy(t_modle.subMsg.device_secret,msgvec[3].c_str());


    if(posvec.size()<4){
        printf("gateway msg is not complete!\r\n");
    }
    else{
        t_modle.subMsg.product_secret = new char[strlen(posvec[3].c_str())+1];
        strcpy(t_modle.subMsg.product_secret,posvec[3].c_str());
    }


    m_subdev.push_back(t_modle);
    g_subdev[addIndex++] = t_modle.subMsg;
}


void        CMqtt::Init_Subdev()
{
    m_SendThread.Start(WorkTaskProc,this,true);

}


int        CMqtt::Init_MQTT_Conn()
{
    int32_t res = STATE_SUCCESS;
    mqtt_handle = NULL;
    dm_handle = NULL;
    subdev_handle = NULL;

    const DeviceInfo & minfo = *this->GetDevice()->GetDeviceInfo();
    string info_secret(minfo.Addressex);
    vector<string> posvec;
    posvec = split(info_secret,"|");


    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    aiot_state_set_logcb(demo_state_logcb);

    res = demo_mqtt_start(&mqtt_handle,&dm_handle,posvec,(void *)this);
    if (res < 0) {
        printf("demo_mqtt_start failed\n");
        return -1;
    }

        subdev_handle = aiot_subdev_init();
    if (subdev_handle == NULL) {
        printf("aiot_subdev_init failed\n");
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    aiot_subdev_setopt(subdev_handle, AIOT_SUBDEVOPT_MQTT_HANDLE, mqtt_handle);
    aiot_subdev_setopt(subdev_handle, AIOT_SUBDEVOPT_RECV_HANDLER, (void *)demo_subdev_recv_handler);

/*    aiot_subdev_dev_t *g_subdev = new aiot_subdev_dev_t[m_subdev.size()];
    for(int i=0;i<m_subdev.size();i++)
    {
        g_subdev[i] = m_subdev[i].subMsg;
    }
    printf("m_subdev size of: %d\r\n",m_subdev.size());

/*    aiot_subdev_dev_t g_subdev[] = {
        {
            "a1n77haT1LD",
            "20190601",
            "6fad69fd9f0ab73686b25071f56d7c34",
            "CEIRDwbKsddPHUO3"
        },
        {
          "a1V2eTHibfd",
          "A5520",
          "6444a04ea873d536bf6e03dcb7508955",
          "xOt7XTbHMPCpnMT7"
        }
    };*/
/*    res = aiot_subdev_send_topo_add(subdev_handle, g_subdev, m_subdev.size());
    if (res < STATE_SUCCESS) {
        printf("aiot_subdev_send_topo_add failed, res: -0x%04X\n", -res);
        aiot_subdev_deinit(&subdev_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }*/
//    sleep(2); //添加子设备与网关topo关系的请求 需要等待时间

    int loginCount = m_subdev.size()/5;
    if((m_subdev.size()%5)>0)
        loginCount++;
    aiot_subdev_dev_t *t_subdev;
    for(int j=0;j<loginCount;j++)
    {
        t_subdev = &g_subdev[0+j*5];
        if(j == (loginCount-1)){
            int ltimes = m_subdev.size()%5;
            if(ltimes==0)
               aiot_subdev_send_batch_login(subdev_handle, t_subdev, 5);
            else
               aiot_subdev_send_batch_login(subdev_handle, t_subdev, ltimes);
        }
        else{
            aiot_subdev_send_batch_login(subdev_handle, t_subdev, 5);
        }
        sleep(0.5);
    }
    m_isSubConn = true;
/*  aiot_subdev_send_batch_login(subdev_handle, g_subdev, m_subdev.size());
    if (res < STATE_SUCCESS) {
        printf("aiot_subdev_send_batch_login failed, res: -0x%04X\n", -res);
        aiot_subdev_deinit(&subdev_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }*/

    sleep(1); //向物联网平台发送子设备上线请求 需要等待时间
    m_isConn = 1;
    return 1;
}

void    CMqtt::Send_dm_data()
{
    if(m_isConn==-1){
       // printf("Not connect Mqtt\r\n");
        return;
    }

    if(ykSendDm)
    {
        time(&ykNewTime);
        if(difftime(ykNewTime, ykOldTime)>60*5)
        {
            char property_params[512] = {0};
            aiot_dm_msg_t msg;
            memset(&msg, 0, sizeof(aiot_dm_msg_t));
            msg.type = AIOT_DMMSG_PROPERTY_POST;
            msg.product_key = "a1TCpTd5oJ8";//"a1V2eTHibfd";
            msg.device_name = (char *)ykDevName.c_str();//"A5520";

            float fdata[16] = {0};
            get_var_data((char *)ykDevName.c_str(),fdata);

            char *propertyFmt ="{\"nActiveEnergy\": %f,\"nUa\": %f,\"nUb\": %f,\"nUc\": %f,\"nIa\": %f,\"nIb\": %f,\"nIc\": %f,\"P\": %f,\"electric_fra\": %f,\"electric_frb\": %f,\"electric_frc\": %f,\"Breaker_control\": %d}";
            snprintf(property_params, sizeof(property_params), propertyFmt
            ,fdata[0]
            ,fdata[1]
            ,fdata[2]
            ,fdata[3]
            ,fdata[4]
            ,fdata[5]
            ,fdata[6]
            ,fdata[7]
            ,fdata[8]
            ,fdata[9]
            ,fdata[10]
            ,(int)fdata[11]
            );
            msg.data.property_post.params = property_params;
            aiot_dm_send(dm_handle, &msg);
            sleep(1);
            ykSendDm = false;
        }

    }

    time(&Newcurtime);
    if(difftime(Newcurtime, oldcurtime)>60*60)
    {
        printf("start send message\r\n");
        time(&oldcurtime);
/*        if(!firstSend){
            printf("first send ignore!\r\n");
            firstSend = true;
            return;
        }*/
        for(int i = 0;i<m_subdev.size();i++)
        {
            char property_params[512] = {0};
            aiot_dm_msg_t msg;
            memset(&msg, 0, sizeof(aiot_dm_msg_t));
            msg.type = AIOT_DMMSG_PROPERTY_POST;
            msg.product_key = m_subdev[i].subMsg.product_key;//"a1V2eTHibfd";
            msg.device_name = m_subdev[i].subMsg.device_name;//"A5520";

            float fdata[16] = {0};
            get_var_data(m_subdev[i].devname,fdata);

            char *propertyFmt ="{\"nActiveEnergy\": %f,\"nUa\": %f,\"nUb\": %f,\"nUc\": %f,\"nIa\": %f,\"nIb\": %f,\"nIc\": %f,\"P\": %f,\"electric_fra\": %f,\"electric_frb\": %f,\"electric_frc\": %f,\"Breaker_control\": %d}";
            snprintf(property_params, sizeof(property_params), propertyFmt
            ,fdata[0]
            ,fdata[1]
            ,fdata[2]
            ,fdata[3]
            ,fdata[4]
            ,fdata[5]
            ,fdata[6]
            ,fdata[7]
            ,fdata[8]
            ,fdata[9]
            ,fdata[10]
            ,(int)fdata[11]
            );
            msg.data.property_post.params = property_params;
            aiot_dm_send(dm_handle, &msg);
            sleep(1);
        }
        return ;
    }



}

void     CMqtt::get_var_data(char * devname,float *fd)
{
    ST_INT varcount = g_pTree->GetNameNodeCount(devname);
    if (varcount <= 0) return;
    ST_CHAR ditname[64] = {0}, fullname[256] = {0};
    for(int j = 0; j < varcount; ++j)
    {
//            memset(fullname, 0, sizeof(fullname) / 2);
//            memset(ditname , 0, sizeof(ditname ) / 2);
        *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

        g_pTree->GetNodeName(devname, j, ditname);

        strcpy(fullname, devname); strcat(fullname, ".");
        strcat(fullname, ditname); strcat(fullname, ".value");
        ST_DUADDR tdd;
        g_pTree->GetNodeAddr(fullname, tdd);
        ST_VARIANT vValue;
            // GetVariableValueByName(&fullname[0],vValue);
        GetVariableValueByAddr(tdd, vValue);
        fd[tdd.addr] = vValue.fVal;
    }
}


ST_VOID *CMqtt::WorkTaskProc(ST_VOID *param)
{
    CMqtt *pd=(CMqtt*)param;
    if(pd->m_isConn==-1){
        pd->Init_MQTT_Conn();
    }
    else
    {
        if(m_isSubConn)
            pd->Send_dm_data();
        else
            pd->Subdev_batch_login();
    }

	sleep(1);
	return 0;
}

void     CMqtt::tranfer_YK_task(int devicId,bool bvalue)
{
    ProtocolTask task;
    task.isTransfer     = true;
    task.transChannelId = -1;
    task.transDeviceId  = devicId;
    Strcpy(task.taskCmd, "devicecontrol");
    task.taskCmdCode = 1;
    task.taskValue   = bvalue;
    Transfer(&task);
}

int     CMqtt::get_devId(char *devName)
{
    string dn(devName);
    int di = -1;
    for(int i = 0;i<m_subdev.size();i++)
    {
        string tdn(m_subdev[i].subMsg.device_name);
        if(dn == tdn)
        {
            di = m_subdev[i].deviceId;
        }
    }
    return di;
}

void     CMqtt::Subdev_batch_login()
{

/*    aiot_subdev_dev_t *g_subdev = new aiot_subdev_dev_t[m_subdev.size()];
    for(int i=0;i<m_subdev.size();i++)
    {
        g_subdev[i] = m_subdev[i].subMsg;
    }
*/
    if(m_isConn==-1){
       // printf("Not connect Mqtt\r\n");
        return;
    }
    SysLogger::GetInstance()->LogWarn ("Restart Batch subdevice.");
    int loginCount = m_subdev.size()/5;
    if((m_subdev.size()%5)>0)
        loginCount++;
    aiot_subdev_dev_t *t_subdev;
    for(int j=0;j<loginCount;j++)
    {
        t_subdev = &g_subdev[0+j*5];
        if(j == (loginCount-1)){
            int ltimes = m_subdev.size()%5;
            if(ltimes==0)
               aiot_subdev_send_batch_login(subdev_handle, t_subdev, 5);
            else
               aiot_subdev_send_batch_login(subdev_handle, t_subdev, ltimes);
        }
        else{
            aiot_subdev_send_batch_login(subdev_handle, t_subdev, 5);
        }
        sleep(0.5);
    }
    m_isSubConn = true;
//    delete[] g_subdev;
    sleep(1); //向物联网平台发送子设备上线请求 需要等待时间
}
