// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "DL645.h"
#include "Device.h"
#include "stdio.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <vector>
#include "syslogger.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dm_api.h"
#include "dm_private.h"
#include "cJSON.h"
/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;



inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
	ST_BYTE bySum = 0x00;
	for(int i = 0; i < len; ++i)
	{
		bySum += pbuf[i];
	}
	return  bySum;
}

Protocol * CreateInstace()
{
	return new CDL645();
}

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] mqtt_basic_demo&a13FN5TplKq
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\r\n");
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\r\n");
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? (char *)("network disconnect") :
                          (char *)("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
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
    CDL645 *tmp = (CDL645 *)args;
    while (tmp->g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(tmp->mqtt_handle);
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
    CDL645 *tmp = (CDL645 *)args;

    while (tmp->g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(tmp->mqtt_handle);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            sleep(1);
        }
    }
    return NULL;
}

/* 用户数据接收处理回调函数 void *dm_handle static*/
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

            cJSON *json,*json_value;
            json = cJSON_Parse(recv->data.async_service_invoke.params);
            json_value = cJSON_GetObjectItem( json , "codeValue" );
            int cValue = json_value->valueint;
            //printf("service code value :%d\n",cValue);
            /* TODO: 以下代码演示如何对来自云平台的异步服务调用进行应答, 用户可取消注释查看演示效果
             *
             * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
             */

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


          dm_handle_t *handle;
          handle = (dm_handle_t *)dm_handle;
          CDL645 *tmp =  (CDL645 *)handle->protocl;
          tmp->m_ykstatue = true;
          tmp->m_ykvalue = 0;
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

            /*
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
            */
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

/* 设备属性上报函数演示 */
int32_t demo_send_property_post(void *dm_handle)
{
    /* 属性上报payload格式, 其中地理位置固定不变, 仅用于演示 */
    char *propertyFmt = "{\"GeoLocation\":{\"Longitude\":120.08,\"Latitude\":30.13,\"Altitude\":39.01},\"BatteryPercentage\":%f, \"Temperature\":%f}";
    /* 温度为0 ~ 60的随机数, 仅用于演示 */
    float temperature = rand() * 1.0 / RAND_MAX * 60;
    /* 电池电量为0 ~ 100的随机数, 仅用于演示 */
    float battery = rand() * 1.0 / RAND_MAX * 100;
    char property_params[256] = {0};
    aiot_dm_msg_t msg;

    snprintf(property_params, sizeof(property_params), propertyFmt, battery, temperature);
    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_POST;
    msg.data.property_post.params = property_params;

    return aiot_dm_send(dm_handle, &msg);
}

/* 事件上报函数演示 */
int32_t demo_send_event_post(void *dm_handle, char *event_id, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_EVENT_POST;
    msg.data.event_post.event_id = event_id;
    msg.data.event_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

/* 演示了获取属性LightSwitch的期望值, 用户可将此函数加入到main函数中运行演示 */
int32_t demo_send_get_desred_requset(void *dm_handle)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_GET_DESIRED;
    msg.data.get_desired.params = "[\"LightSwitch\"]";

    return aiot_dm_send(dm_handle, &msg);
}

/* 演示了删除属性LightSwitch的期望值, 用户可将此函数加入到main函数中运行演示 */
int32_t demo_send_delete_desred_requset(void *dm_handle)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_DELETE_DESIRED;
    msg.data.get_desired.params = "{\"LightSwitch\":{}}";

    return aiot_dm_send(dm_handle, &msg);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDL645::CDL645()
{
    m_readIndex = 0;
	memset (m_addrarea, 0, sizeof(m_addrarea));
	mqisinit = -1;
	m_ykstatue = false;
	m_ykvalue = 0;
}

CDL645::~CDL645()
{
}

void CDL645::Init()
{
    //mqisinit = InitMQTThandle();
}

void CDL645::Uninit()
{
}

bool CDL645::IsSupportEngine(ST_INT engineType)
{
	return true;
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

int CDL645::InitMQTThandle()
{
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com";//"203.107.45.14";//"iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* TODO: 替换为自己设备的三元组
    {
    "ProductKey": "a1V6hv8BEUi",
    "DeviceName": "test01",
    "DeviceSecret": "MT7zWR6XQUgDZZsL8rHoIAnleN2LTWIp"
    }
    */
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    string addrex(info.Addressex);
    vector<string> msgvec;
    msgvec = split(addrex,"|");
 /*   printf("begin printlf msg:\r\n");
    for(int i = 0; i<msgvec.size();i++)
    {
        printf(msgvec[i].c_str());
        printf("\r\n");
    }*/
    if(msgvec.size()<4)
        return -1;
    char *product_key       = (char *)msgvec[1].c_str();//"a1V6hv8BEUi";
    char *device_name       = (char *)msgvec[2].c_str();//"test01";
    char *device_secret     = (char *)msgvec[3].c_str();//"MT7zWR6XQUgDZZsL8rHoIAnleN2LTWIp";

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);

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
        printf("aiot_mqtt_init failed\r\n");
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
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 创建DATA-MODEL实例 */
    dm_handle = aiot_dm_init();
    if (dm_handle == NULL) {
        printf("aiot_dm_init failed\r\n");
        return -1;
    }
    // void CDL645::demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
    //aiot_dm_recv_handler_t
    /* 配置MQTT实例句柄 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    /* 配置消息接收处理回调函数 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)demo_dm_recv_handler);
    //(dm_handle_t *)dm_handle->recv_handler = (aiot_dm_recv_handler_t)&CDL645::demo_dm_recv_handler;
    aiot_dm_setopt(dm_handle,AIOT_DMOPT_PROTOCOL,(void *)this);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
        return -1;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, this);
    if (res < 0) {
        SysLogger::GetInstance()->LogWarn("pthread_create demo_mqtt_process_thread failed: %d\r\n", res);
        printf("pthread_create demo_mqtt_process_thread failed: %d\r\n", res);
        return -1;
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, this);
    if (res < 0) {
        SysLogger::GetInstance()->LogWarn("pthread_create demo_mqtt_recv_thread failed: %d\r\n", res);
        printf("pthread_create demo_mqtt_recv_thread failed: %d\r\n", res);
        return -1;
    }

    return 1;
}


int32_t CDL645::demo_send_property_post(void *dm_handle)
{
    /* 属性上报payload格式, 其中地理位置固定不变, 仅用于演示 */
    //char *propertyFmt = "{\"GeoLocation\":{\"Longitude\":120.08,\"Latitude\":30.13,\"Altitude\":39.01},\"BatteryPercentage\":%f, \"Temperature\":%f}";
    char *propertyFmt = "{\"nActiveEnergy\":100,\"nUa\":%f, \"nUb\":%f, \"nUc\":%f, \"nIa\":%f, \"nIb\":%f,\"nIc\":%f,\"Breaker_control\":%d}";
    /* 温度为0 ~ 60的随机数, 仅用于演示 */
    float temperature = rand() * 1.0 / RAND_MAX * 60;
    /* 电池电量为0 ~ 100的随机数, 仅用于演示 */
    float battery = rand() * 1.0 / RAND_MAX * 100;
    float battery1 = rand() * 1.0 / RAND_MAX * 100;
    char property_params[256] = {0};
    aiot_dm_msg_t msg;

    snprintf(property_params, sizeof(property_params)
            , propertyFmt
            , battery
            , temperature
            ,battery1
            ,battery1
            ,battery1
            ,battery1
            ,1);
    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_POST;
    msg.data.property_post.params = property_params;

    return aiot_dm_send(dm_handle, &msg);
}

void CDL645::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 8, 2000);
		if(len < 8) {
			ShowMessage ("Insufficient data length");
			return;
		}
		ST_INT star = 0;
		for(; star < len; ++star) {
			if(pbuf[star] == 0x68)
				break;
		}
		if(len == star) {
			ShowMessage ("Garbled code, clear buffer.");
			this->GetCurPort()->Clear();
			return;
		}
		if(star > 0)
		{
			this->GetCurPort()->ReadBytes(pbuf, star);
		}

        if (this->GetDevice()->GetDeviceInfo()->DataAreasCount <= 0)
        {
            ShowMessage("No Point Table.");
            this->GetCurPort()->Clear();
            return;
        }

		len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
		if((pbuf[0] == 0x68) && (pbuf[7] == 0x68))
		{
			ST_INT ndatalen = pbuf[9] + 12;
			if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) == ndatalen)
			{
                if (memcmp(pbuf + 1, m_addrarea, sizeof(m_addrarea)))
				{
					this->ShowMessage("Address not match.");
					return;
				}
				if (get_check_sum(pbuf, ndatalen - 2) == pbuf[ndatalen - 2])
				{
					readed = ndatalen;
					return;
				}
				else {
					ShowMessage("Check error!");
					this->ShowRecvFrame(pbuf,len);
					this->GetCurPort()->Clear();
					return;
				}
			}
		}
		else
		{
			ShowMessage("Sync header error.");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
			return;
		}
	}
}

bool CDL645::OnSend()
{

    if(mqisinit==-1){
        mqisinit = InitMQTThandle();
    }
/*    else{
    res = demo_send_property_post(dm_handle);
    if (res < 0) {
        printf("demo_send_property_post failed: -0x%04X\r\n", -res);
        }
    }*/
    if(m_ykstatue)
    {
        ShowMessage("Send YK ");
        SendYk(m_ykvalue);
        m_ykstatue = false;
        return true;
    }
	switch(m_readIndex) {
		case 0: ReadData(0x00000000); break; // 当前组合有功总电能
		case 1: ReadData(0x0201ff00); break; // 电压
		case 2: ReadData(0x0202ff00); break; // 电流
		case 3: ReadData(0x0203ff00); break; // 有功功率
		case 4: ReadData(0x04000503); break; // 电表运行状态字3
 		default: break;
	}
	m_readIndex = ((++m_readIndex) % 5); // 19+1
	return true;
}

bool CDL645::OnProcess (ST_BYTE* pbuf, ST_INT len)
{

	if(pbuf[8] == 0x91) {
        unsigned long datatype = 0;
        memcpy(&datatype, pbuf + 10, sizeof(datatype));
        switch (datatype) {
            case 0x00000000:
            case 0x33333333:{
                //const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[0];
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0x70) >> 4) * 10000000)*0.01;

                    if ( (pbuf[17]-0x33) & 0x80 )
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                    }
                //if (itemref.coeficient != 0)
                //    fvalue = fvalue * itemref.coeficient;
                this->update_All_platform(0, fvalue);
            }break;

            case 0x0201FF00:
            case 0x35343233: {
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0xF0)>>4)*1000)*0.1;
                    this->update_All_platform(i +  1, fvalue);
                }
            } break;

            case 0x0202FF00:
            case 0x35353233: {
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.001;

                    if ((pbuf[16+i*3]-0x33) & 0x80)
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                    }
                    this->update_All_platform(i +  4, fvalue);
                }
            } break;

            case 0x0203FF00:
            case 0x35363233: {
                float fvalue = 0;
                for(int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;

                    if ((pbuf[16+i*3] -0x33) & 0x80)
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                    }
                    this->update_All_platform(i +  7, fvalue);
                }
            } break;

            case 0x04000503:
            case 0x37333836: {
                uint8_t bith = pbuf[15] - 0x33;
                uint8_t bitl = pbuf[14] - 0x33;
                ST_BYTE value = 0;
                value = (bitl & 0x10)? 1:0;
                this->update_All_platform(11,value);
            } break;
        }

	}
	else if (pbuf[8] == 0xD1){
        this->ShowMessage("Secondary station error ack.");
	}
	return true;
}

void    CDL645::ReadData(ST_UINT32 wAddr) //参数由高到低，写出由低到高  主站请求帧
{
	ST_BYTE sendbuf[32] = {0};
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[0] = 0x68;

	m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
	m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
	m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 01, m_addrarea, sizeof(m_addrarea));

	sendbuf[7] = 0x68;
	sendbuf[8] = 0x11;
	sendbuf[9] = 0x04;

	ST_UINT16 wTempH = (wAddr & 0xffff0000) >> 16;
	ST_UINT16 wTempL =  wAddr & 0x0000ffff;
	sendbuf[10] = ( wTempL&0x00ff)    +0x33;
	sendbuf[11] = ((wTempL&0xff00)>>8)+0x33;
	sendbuf[12] = ( wTempH&0x00ff)    +0x33;
	sendbuf[13] = ((wTempH&0xff00)>>8)+0x33;

    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 14; ++i)
	{
		bySum += sendbuf[i];
	}
	sendbuf[14] = bySum;
	sendbuf[15] = 0x16;
	this->Send(sendbuf, 16);
}

//68 06 10 24 05 19 20 68 1C 10 35 33 33 33 33 33 33 33 4D 33 33 33 33 33 33 33 C0 16
void  CDL645::SendYk(int isbool)
{
   ST_BYTE sendbuf[32] = {0};
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[0] = 0x68;

	m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
	m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
	m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 01, m_addrarea, sizeof(m_addrarea));

	sendbuf[7] = 0x68;
	sendbuf[8] = 0x1C;
	sendbuf[9] = 0x10;

	sendbuf[10] = 0x35;
	sendbuf[11] = 0x33;
	sendbuf[12] = 0x33;
	sendbuf[13] = 0x33;

	sendbuf[14] = 0x33;
	sendbuf[15] = 0x33;
	sendbuf[16] = 0x33;
	sendbuf[17] = 0x33;

	sendbuf[18] = isbool?0x4E:0x4D;

	sendbuf[19] = 0x33;
	sendbuf[20] = 0x33;
	sendbuf[21] = 0x33;
	sendbuf[22] = 0x33;
	sendbuf[23] = 0x33;
	sendbuf[24] = 0x33;
	sendbuf[25] = 0x33;

    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 26; ++i)
	{
		bySum += sendbuf[i];
	}
	sendbuf[26] = bySum;
	sendbuf[27] = 0x16;
	this->Send(sendbuf, 28);
}

void  CDL645::update_All_platform(int id,float fvalue)
{
    char property_params[256] = {0};
    aiot_dm_msg_t msg;
    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_POST;
    switch(id){
        case 0:{
            char *propertyFmt = "{\"nActiveEnergy\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 1:{
            char *propertyFmt = "{\"nUa\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 2:{
            char *propertyFmt = "{\"nUb\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 3:{
            char *propertyFmt = "{\"nUc\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 4:{
            char *propertyFmt = "{\"nIa\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 5:{
            char *propertyFmt = "{\"nIb\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 6:{
            char *propertyFmt = "{\"nIc\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 7:{
            char *propertyFmt = "{\"electric_fra\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 8:{
            char *propertyFmt = "{\"electric_frb\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 9:{
            char *propertyFmt = "{\"electric_frc\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 10:{
            char *propertyFmt = "{\"P\": %f}";
            snprintf(property_params, sizeof(property_params), propertyFmt,fvalue);
            msg.data.property_post.params = property_params;
        }break;
        case 11:{
            char *propertyFmt = "{\"Breaker_control\": %d}";
            snprintf(property_params, sizeof(property_params), propertyFmt,(int)fvalue);
            msg.data.property_post.params = property_params;
        }break;
        default:{
            ShowMessage("id not matching!");
        }
    }
    aiot_dm_send(dm_handle, &msg);
    this->UpdateValue(id,fvalue);
}
