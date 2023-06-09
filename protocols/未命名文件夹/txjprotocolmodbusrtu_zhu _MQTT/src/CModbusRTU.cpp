#include "CModbusRTU.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

#define sDebug	if (true) wedoDebug (SysLogger::GetInstance()).noquote
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

static const uint16_t crc16_table[256] =
{
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
	0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
	0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
	0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
	0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
	0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
	0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
	0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
	0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
	0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
	0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
	0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
	0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
	0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
	0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
	0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
	0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
	0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
	0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
	0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
	0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
	0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
	0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
	0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
	0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
	0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
	0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
	0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

inline ST_BYTE FromBCD_BYTE(ST_BYTE value)
{
	return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

ST_UINT16 FromBCD_WORD(ST_UINT16 value)
{
	return (ST_UINT16)FromBCD_BYTE((value & 0xFF00) >> 8) * 100 + (ST_UINT16)FromBCD_BYTE(value & 0x00FF);
}

ST_UINT32 FromBCD_DWORD(ST_UINT32 value)
{
	return (ST_UINT32)FromBCD_WORD((value & 0xFFFF0000) >> 16) * 10000 + (ST_UINT32)FromBCD_WORD(value & 0x0000FFFF);
}

inline ST_BYTE TOBCD_BYTE(ST_BYTE value)
{
	return ((value / 10) << 4) + (value % 10);
}

ST_UINT16 TOBCD_WORD(ST_UINT16 value)
{
	return ((ST_UINT16)TOBCD_BYTE((value & 0xFF00) >> 8) << 8) + (ST_UINT16)TOBCD_BYTE(value & 0x00FF);
}

ST_UINT32 TOBCD_DWORD(ST_UINT32 value)
{
	return ((ST_UINT32)TOBCD_WORD((value & 0xFFFF0000) >> 16) << 16) + (ST_UINT32)TOBCD_WORD(value & 0x0000FFFF);
}

/*ST_UINT GetCRC16(const ST_BYTE *pbData, ST_INT nSize)
{
	ST_UINT crc = 0xffff;
	for (ST_INT i = 0; i < nSize; i++)
	{
		crc = m_crc16_table[(crc & 0xff) ^ (*pbData++)] ^ (crc >> 8);
	}
	return crc;
}*/

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

class convert
{
public:

	template<typename T>
	static T bytes_to (const void * bytes, size_t len)
	{
		T value;
		memcpy (&value, bytes, min(sizeof(T), len));
		return value;
	}

};


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
    CModbusRTU *tmp = (CModbusRTU *)args;
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
    CModbusRTU *tmp = (CModbusRTU *)args;

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
 /*         {
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
          CModbusRTU *tmp =  (CModbusRTU *)handle->protocl;
          tmp->m_ykstatue = true;
          tmp->m_ykvalue = cValue;*/
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



CModbusRTU::CModbusRTU()
{
    //ctor

}

CModbusRTU::~CModbusRTU()
{
    //dtor
}

CModbusRTU* CreateInstace()
{
    return new CModbusRTU();
}

ST_BOOLEAN	CModbusRTU::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	CModbusRTU::Init()
{
	m_bTask = false;
	m_curreadIndex = 0;
	m_readIndex = 0;
	InitMQTThandle();
	time(&Newcurtime);
	time(&oldcurtime);
}

void	CModbusRTU::Uninit()
{

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



int CModbusRTU::InitMQTThandle()
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
    // void CModbusRTU::demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
    //aiot_dm_recv_handler_t
    /* 配置MQTT实例句柄 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    /* 配置消息接收处理回调函数 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)demo_dm_recv_handler);
    //(dm_handle_t *)dm_handle->recv_handler = (aiot_dm_recv_handler_t)&CModbusRTU::demo_dm_recv_handler;
    aiot_dm_setopt(dm_handle,AIOT_DMOPT_PROTOCOL,(void *)this);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        if(!isreport)
        {
            isreport = true;
            printf("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
            SysLogger::GetInstance()->LogWarn("%s diconnect server ,error code :-0x%04X\r\n",this->GetDevice()->GetDeviceInfo()->DeviceName,-res);
        }
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



void	CModbusRTU::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	if(! this->GetCurPort())
		return;
	if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
	{
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	if(len < 5) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	for(; star < len; ++star) {
		if(pbuf[star] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
				break;
	}
	if(star > 0) {
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	if(star == len) {
		//全是乱码
		ShowMessage ("Garbled code, clear buffer.");
		this->GetCurPort()->Clear();
		return;
	}
	len = this->GetCurPort()->PickBytes(pbuf, 5, 2000);
	ST_BYTE fuccode = pbuf[1 + star];
	if((fuccode & 0xF0) == 0x80)
	{
		len = 5;
	}
	else if(fuccode == 0x05)
	{
		len = 8;
	}
	else if((fuccode == 0x06) && m_bTask)
	{
		len = m_curTask.taskParamLen + 6;
	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
	{
		//读数据返回
		ST_BYTE readCount = pbuf[2 + star];
		len = readCount + 5;
	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].writeCode)
	{
		//写数据返回
		len = 8;
	}
	else
	{
		ShowMessage ("Not Found Function Code!");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 2000);
	if(nlen == len)
	{
		this->GetCurPort()->ReadBytes(pbuf, len);
		ST_UINT16 wCRC = get_crc16(&pbuf[0], len-2);
		ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
	//	if(wCRC == nCRC)
		{
			readed = len;
			return;
		}
	//	else
	//	{
	//		ShowMessage ("Check error!");
	//		this->GetCurPort()->Clear();
	//		return;
	//	}
	}
	else
	{
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN	CModbusRTU::OnSend()
{
    if (this->GetCurPort())
        this->GetCurPort()->Clear();

	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"singleread"))
		{
			SendReadCmd((ST_BYTE)m_curTask.taskCmdCode,m_curTask.taskAddr,m_curTask.taskAddr1);
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)
				SendPreYK(m_curTask.taskAddr,m_curTask.taskValue);
			else if(m_curTask.taskCmdCode == 1)
				SendYK(m_curTask.taskAddr,m_curTask.taskValue);
			else if(m_curTask.taskCmdCode == 2)
				SendReadCmd((ST_BYTE)m_curTask.taskCmdCode,m_curTask.taskAddr,m_curTask.taskAddr1);
			else
			{
                m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                memset(&m_curTask,0,sizeof(m_curTask));
				return false;
			}
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"singlewrite"))
		{
			if(m_curTask.taskCmdCode == 2)
			{
				ST_UINT16 wIndex = m_curTask.taskParam[1];
				wIndex = (wIndex<<8)| m_curTask.taskParam[0];
				SendYT(m_curTask.taskAddr,wIndex,m_curTask.taskValue?1:0);
			}
			else
				SendSingleWriteCmd((ST_FLOAT)m_curTask.taskValue,m_curTask.taskAddr,m_curTask.taskAddr1);
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"multiwrite"))
		{
			//写定值
			SendWriteCmd(m_curTask.taskParam,m_curTask.taskParamLen,m_curTask.taskAddr);
			m_bTask = true;
		}
		return true;
	}
	const DeviceInfo* info = this->GetDevice()->GetDeviceInfo();
	if(info && info->DataAreasCount > 0)
	{
		if (m_readIndex >= info->DataAreasCount)
			m_readIndex = 0;
		m_curreadIndex = m_readIndex++;
		SendReadCmd(info->DataAreas[m_curreadIndex].readCode, (ST_UINT16)info->DataAreas[m_curreadIndex].addr,
			       info->DataAreas[m_curreadIndex].dataUnitLen > 1 ?
			       info->DataAreas[m_curreadIndex].len / info->DataAreas[m_curreadIndex].dataUnitLen : info->DataAreas[m_curreadIndex].len);
	}
	return true;
}

ST_BOOLEAN	CModbusRTU::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
	if(m_bTask)
	{
		if(!strcmp(m_curTask.taskCmd,"singlewrite") || !strcmp(m_curTask.taskCmd,"multiwrite"))
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"singleread"))
		{
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDataLen = pbuf[2];
			memcpy(m_curTask.taskResult.resultData,&pbuf[3],m_curTask.taskResult.resultDataLen);
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
	}
	else if(m_curreadIndex >= 0 && m_curreadIndex < this->GetDevice()->GetDeviceInfo()->DataAreasCount
			&& pbuf[1] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
	{
		ST_BYTE count = pbuf[2];
		ProcessMemory(&pbuf[3], count);
	}
	update_aliyun_mqtt();
	return true;
}

void CModbusRTU::ProcessMemory(ST_BYTE* buf, ST_BYTE count)
{
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].itemCount;
	for(ST_INT k = 0; k < itemsize; k++)
	{
		const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[k];
		if(count < (itemref.addr + itemref.dataLen))
			continue;

		switch(itemref.dataType) {
		case VALType_Char :
		case VALType_SByte:
			ProcessMemorySByte    (buf, itemref); break;
		case VALType_Int16:
			ProcessMemoryInt16    (buf, itemref); break;
		case VALType_Int32:
			ProcessMemoryInt32    (buf, itemref); break;
		case VALType_Byte :
			ProcessMemoryByte     (buf, itemref); break;
		case VALType_UInt16:
			ProcessMemoryUInt16   (buf, itemref); break;
		case VALType_UInt32:
			ProcessMemoryUInt32   (buf, itemref); break;
		case VALType_Float:
			ProcessMemorySingle   (buf, itemref); break;
		case VALType_Boolean:
			ProcessMemoryBoolean  (buf, itemref); break;
		case VALType_String:
			ProcessMemoryString   (buf, itemref); break;
		case VALType_Binary:
			ProcessMemoryBytes    (buf, itemref); break;
		case VALType_Double:
			ProcessMemoryDouble   (buf, itemref); break;
		case VALType_Decimal:
			ProcessMemoryDecimal  (buf, itemref); break;
		case VALType_DateTime:
			ProcessMemoryDateTime (buf, itemref); break;
		case VALType_Int64:
			ProcessMemoryInt64    (buf, itemref); break;
		case VALType_UInt64:
			ProcessMemoryUInt64   (buf, itemref); break;
		default: break;
		}
	}
}

inline ST_UINT16 bswap16 (ST_UINT16 value)
{
	return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}
inline ST_UINT32 bswap32 (ST_UINT32 value)
{
	return (((ST_UINT32)bswap16(value & 0x0000FFFF) << 16) | bswap16((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 bswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)bswap32(value & 0x00000000FFFFFFFF) << 32) | bswap32((value & 0xFFFFFFFF00000000) >> 32));
}
inline ST_UINT32 wswap32 (ST_UINT32 value)
{
	return (((value & 0x0000FFFF) << 16) | ((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 wswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)wswap32(value & 0x00000000FFFFFFFF) << 32) | wswap32((value & 0xFFFFFFFF00000000) >> 32));
}

void  CModbusRTU::ProcessMemorySByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if(itemref.endBit - itemref.beginBit < 7)
		{
			value = (value & (0xFF << (itemref.beginBit -1))) & (0xFF >> (8 - itemref.endBit));
			value = value >> (itemref.beginBit - 1);
		}
		if(itemref.coeficient < 0)
		{
			value = !value;
		}
		this->UpdateValue(itemref.id, (ST_BYTE)value);
	//	if((itemref.id == 25 ) || (itemref.id == 26) ||(itemref.id == 27) ||(itemref.id == 28) )
	//	{
    //         this->UpdateValue(34, (ST_BYTE)value);
	//	}
		//this->UpdateValue(k,(ST_BYTE)value);
	}
	else if(itemref.dataLen == 2)
	{
		ST_INT16 value = 0x0000;
		if((itemref.codeType == 1))
		{
			memcpy (&value, buf + itemref.addr, sizeof (value));
			*((ST_UINT16*)&value) = bswap16(*((ST_UINT16*)&value));
		}
		else
		{
			memcpy (&value, buf + itemref.addr, sizeof (value));
		}
		if(itemref.endBit - itemref.beginBit < 15)
		{
			value = (value & (0xFFFF << (itemref.beginBit -1))) & (0xFFFF >> (16 - itemref.endBit));
			value = value >> (itemref.beginBit - 1);
		}
		if(itemref.coeficient < 0)
		{
			value = !value;
		}
		this->UpdateValue(itemref.id, (ST_BYTE)value);
	}
}

void  CModbusRTU::ProcessMemoryInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_FLOAT)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_WORD(value);
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_WORD(bswap16(value));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = bswap16(value);
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_DWORD(value);
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_DWORD(bswap32(value));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = bswap32(value);
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = htonl1(value);
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = wswap32(value);
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* dvalue = (ST_UINT16*)&value;
			fvalue = (ST_FLOAT)(dvalue[1]*10000 + dvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT64 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* dvalue = (ST_UINT16*)&value;
			fvalue = (ST_FLOAT)(bswap16(dvalue[3])*1000000000000 + bswap16(dvalue[2])*100000000 + bswap16(dvalue[1])*10000 + bswap16(dvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
	update_All_platform(itemref.id, fvalue);
}

void  CModbusRTU::ProcessMemoryInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_DOUBLE)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_WORD(value);
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_WORD(bswap16(value));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = bswap16(value);
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_DWORD(value);
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = FromBCD_DWORD(bswap32(value));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
            ST_UINT32 value = 0;
            memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = bswap32(value);
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = htonl1(value);
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			value = wswap32(value);
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_DOUBLE)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT64 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_DOUBLE)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
	update_All_platform(itemref.id, fvalue);
}

void  CModbusRTU::ProcessMemoryByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ProcessMemorySByte(buf, itemref);
}

void  CModbusRTU::ProcessMemoryUInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue  = (ST_FLOAT)(value);
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_WORD(value);
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_WORD(bswap16(value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 1))
		{
            ST_UINT16 value = 0;
            memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)bswap16(value);
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_DWORD(value);
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_DWORD(bswap32(value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = bswap32(value);
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = htonl1(value);
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = wswap32(value);
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_FLOAT)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT64 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_FLOAT)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
	update_All_platform(itemref.id, fvalue);
}

void  CModbusRTU::ProcessMemoryUInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_DOUBLE)(value);
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_WORD(value);
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_WORD(bswap16(value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = bswap16(value);
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_DWORD(value);
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = 0;
			memcpy(&value, buf + itemref.addr, sizeof(value));
			fvalue = FromBCD_DWORD(bswap32(value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = bswap32(value);
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = htonl1(value);
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = wswap32(value);
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_DOUBLE)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT64 value = 0;
			memcpy (&value, buf + itemref.addr, sizeof(value));
			ST_UINT16* pbcdvalue = (ST_UINT16*)&value;
			fvalue = (ST_DOUBLE)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
}

void  CModbusRTU::ProcessMemorySingle(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_FLOAT)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen)));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen);
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen)));
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen);
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
	}

	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
}

void  CModbusRTU::ProcessMemoryBoolean(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_BYTE value = (ST_BOOLEAN)(*(buf + itemref.addr));
	this->UpdateValue(itemref.id, value);
}

void  CModbusRTU::ProcessMemoryString(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  CModbusRTU::ProcessMemoryBytes(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  CModbusRTU::ProcessMemoryDouble(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue  = (ST_DOUBLE)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen)));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen);
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(convert::bytes_to<uint16_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen)));
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen);
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);

		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(convert::bytes_to<uint32_t>(buf + itemref.addr, itemref.dataLen));
			fvalue = *((ST_FLOAT*)&value);
		}
	}
	else if (itemref.dataLen == 8)
	{
		switch (itemref.codeType) {
			case 0: {
				ST_UINT64 value = convert::bytes_to<uint64_t>(buf + itemref.addr, itemref.dataLen);
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
			case 1: {
				ST_UINT64 value = bswap64 (convert::bytes_to<uint64_t>(buf + itemref.addr, itemref.dataLen));
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
			case 4: {
			} break;
			case 5: {
				ST_UINT64 value = wswap64 (convert::bytes_to<uint64_t>(buf + itemref.addr, itemref.dataLen));
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
		}
	}

	if(itemref.coeficient != 0)
		fvalue = fvalue * itemref.coeficient;

	this->UpdateValue(itemref.id, fvalue);
	//this->UpdateValue(k,fvalue);
}

void  CModbusRTU::ProcessMemoryDecimal(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  CModbusRTU::ProcessMemoryDateTime(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
    if(itemref.dataLen == 8)
    {

    }
}

static bool BytesToUInt64 (const ST_BYTE* buf, ST_INT codetype, ST_UINT64& value)
{
	switch (codetype) {
		case 0: {
			memcpy (&value, buf, sizeof(value));
		} return true;
		case 1: {
			value = bswap64 (convert::bytes_to<uint64_t>(buf, sizeof(uint64_t)));
		} return true;
		case 4: {
		} return false;
		case 5: {
			value = wswap64 (convert::bytes_to<uint64_t>(buf, sizeof(uint64_t)));
		} return true;
	}
	return false;
}

void  CModbusRTU::ProcessMemoryInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if (itemref.dataLen < 8) {
		ProcessMemoryInt32 (buf, itemref);
		return;
	}
	ST_INT64   value  = 0;
	ST_UINT64& valref = *((ST_UINT64*)&value);
	if (BytesToUInt64 (buf + itemref.addr, itemref.codeType, valref)) {
		if(itemref.coeficient != 0)
			value *= itemref.coeficient;
		this->UpdateValue(itemref.id, (ST_DOUBLE)value);
	}
}

void  CModbusRTU::ProcessMemoryUInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if (itemref.dataLen < 6) {
		ProcessMemoryUInt32 (buf, itemref);
		return;
	}
	ST_UINT64 value = 0;
	if (itemref.dataLen == 6) {
		switch (itemref.codeType) {
			case 0: {
			memcpy (&value, buf + itemref.addr, 6);
			if(itemref.coeficient != 0)
				value *= itemref.coeficient;
			this->UpdateValue(itemref.id, (ST_DOUBLE)value);
			} return;
			case 1: {
			value = (ST_UINT64)(buf[itemref.addr    ]) * 0x10000000000
				  + (ST_UINT64)(buf[itemref.addr + 1]) * 0x100000000
				  + (ST_UINT64)(buf[itemref.addr + 2]) * 0x1000000
				  + (ST_UINT64)(buf[itemref.addr + 3]) * 0x10000
				  + (ST_UINT64)(buf[itemref.addr + 4]) * 0x100
				  + (ST_UINT64)(buf[itemref.addr + 5]);
			if(itemref.coeficient != 0)
				value *= itemref.coeficient;
			this->UpdateValue(itemref.id, (ST_DOUBLE)value);
			} return;
		}
	}

	if (BytesToUInt64 (buf + itemref.addr, itemref.codeType, value)) {
		if(itemref.coeficient != 0)
			value *= itemref.coeficient;
		this->UpdateValue(itemref.id, (ST_DOUBLE)value);
	}
}

void CModbusRTU::SendReadCmd(ST_BYTE code,ST_UINT readAddr,ST_UINT count)
{
	ST_BYTE sendbuf[8];
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = code;
	FillWORD(sendbuf + 2,readAddr);
	FillWORD(sendbuf + 4,count);
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);
}

void CModbusRTU::SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
	ST_BYTE sendbuf[32] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x05;
    if(bIsOn) writeAddr = 0x070B;
	else writeAddr = 0x070C;
    sendbuf[2] = (writeAddr&0xff00)>>8;
	sendbuf[3] = writeAddr&0x00ff;
	sendbuf[4] = (bIsOn ? 0xFF: 0x00);
	sendbuf[5] = 0x00;
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);

}

void CModbusRTU::SendPreYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
	/*ST_BYTE sendbuf[256];
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x05;
	if(bIsOn) writeAddr = 0x070B;
	else writeAddr = 0x070C;
    sendbuf[2] = (writeAddr&0xff00)>>8;
	sendbuf[3] = writeAddr&0x00ff;
	sendbuf[4] = 0xFF;
	sendbuf[5] = 0x00;

	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);*/
	m_curTask.taskResult.resultCode = 0;
    m_curTask.isTransfer = 1;
    Transfer(&m_curTask);
    Memset(&m_curTask, 0, sizeof(m_curTask));
}

void CModbusRTU::SendYT(ST_UINT writeAddr,ST_UINT wIndex,ST_BOOLEAN bIsOn)
{
	ST_BYTE sendbuf[256];
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x05;
	FillWORD(sendbuf + 2,writeAddr);
	if(bIsOn)
		sendbuf[4] = 0xFF;
	else
		sendbuf[4] = 0x00;
	sendbuf[5] = 0x00;
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);

}
void  CModbusRTU::SendSingleWriteCmd(ST_FLOAT data,ST_INT readAddr,ST_INT nType)
{
//	this->OnShowMsg("-----------------------------------------设置单点值",0);
	ST_BYTE sendbuf[256];
	ST_INT len = 0;

	if(nType == 0) //两个字节WORD
	{
		sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
		sendbuf[1] = 0x06;
		FillWORD(sendbuf + 2,readAddr);
		FillWORD(sendbuf + 4,data);
		len = 6;
		*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
		this->Send(sendbuf,len + 2);
	}
	else if(nType == 1)//DWORD
	{
		sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
		sendbuf[1] = 0x06;
		FillWORD(sendbuf + 2,readAddr);
		ST_UINT32 dwTmep = (ST_UINT32)data;
		sendbuf[4] = (dwTmep&0x0000ff00)>>8;
		sendbuf[5] = (dwTmep&0x000000ff);
		sendbuf[6] = (dwTmep&0xff000000)>>24;
		sendbuf[7] = (dwTmep&0x00ff0000)>>16;
		len = 8;
		*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
		this->Send(sendbuf,10);
	}
	else if(nType == 2)//float
	{
		sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
		sendbuf[1] = 0x10;
		FillWORD(sendbuf + 2,readAddr);
		ST_BYTE bTemp[4];
		memcpy(&bTemp[0],&data,sizeof(ST_FLOAT));
		sendbuf[4] = 0x00;
		sendbuf[5] = 0x02;
		sendbuf[6] = 0x04;
		sendbuf[7] = bTemp[1];
		sendbuf[8] = bTemp[0];
		sendbuf[9] = bTemp[3];
		sendbuf[10] = bTemp[2];
		len = 11;
		*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
		this->Send(sendbuf,13);
	}
}

void  CModbusRTU::SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen,ST_UINT addr)
{
	if((dataLen>=1024) || (dataLen<=0)) return;
	ST_BYTE sendbuf[1024];
	for(ST_INT i=0;i<1024;i++)
		sendbuf[i] = 0xff;
	ST_INT len = 0;
	ST_INT ndataLen = (dataLen%2)?(dataLen/2+1):(dataLen/2);
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x10;
	FillWORD(sendbuf + 2,addr);
	FillWORD(sendbuf + 4,ndataLen);
	sendbuf[6] = ndataLen*2;
	memcpy(sendbuf + 7,pData,dataLen);
	len = 7 + ndataLen*2;
	*(ST_UINT16*)(sendbuf + len) = get_crc16(sendbuf,len);
	this->Send(sendbuf,len + 2);
}

void CModbusRTU::FillWORD(ST_BYTE* buf,ST_UINT v)
{
	ST_BYTE* pv = (ST_BYTE*)&v;
	buf[0] = pv[1];
	buf[1] = pv[0];
}

ST_UINT CModbusRTU::GetAppWORDValue(ST_UINT v,ST_INT codetype)
{
	if(codetype == 1 || codetype == 4)
	{
		return bswap16(v);
	}
	else if(codetype == 2)
	{
		return FromBCD_WORD(v);
	}
	return v;
}

ST_UINT32 CModbusRTU::htonl1(ST_UINT32 dv)
{
	ST_UINT16 dwHTemp = (ST_UINT16)((dv&0xffff0000)>>16);
	ST_UINT16 dwLTemp = (ST_UINT16)(dv&0x0000ffff);
	ST_UINT16 wLTemp  = bswap16(dwLTemp);
	ST_UINT16 wHTemp  = bswap16(dwHTemp);
	ST_UINT32 dwTemp = wHTemp;
	return ((dwTemp<<16)|wLTemp);
}

void  CModbusRTU::SendWriteCmd(ST_UCHAR* pData,ST_UINT dataLen)
{
	ST_BYTE sendframe[1024];
	if(dataLen>=1024) return;
	for(ST_UINT i = 0; i < dataLen; i++)
	{
		sendframe[i] = pData[i];
	}
	this->Send(sendframe, dataLen);
}

void  CModbusRTU::update_All_platform(int id,float fvalue)
{

    m_devData[id] = fvalue;
    this->UpdateValue(id,fvalue);
}

void   CModbusRTU::update_aliyun_mqtt()
{
    this->ShowMessage("enter update_aliyun_mqtt1");
    //Newcurtime = clock();
    time(&Newcurtime);
    if(difftime(Newcurtime, oldcurtime)>60*5)
    {
        this->ShowMessage("enter Send mqtt time");
        //oldcurtime = clock();
        time(&oldcurtime);
        //first_update = true;
        if(mqisinit==-1)
            return ;

        char property_params[512] = {0};
        aiot_dm_msg_t msg;
        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_PROPERTY_POST;

        char *propertyFmt =
		"{\"nUa\": %f,\"nUb\": %f,\"nUc\": %f,\"nIa\": %f,\"nIb\": %f,\"nIc\": %f,\"nActiveEnergy\": %f}";
        snprintf(property_params, sizeof(property_params), propertyFmt
        ,m_devData[0]
        ,m_devData[1]
        ,m_devData[2]
        ,m_devData[3]
        ,m_devData[4]
        ,m_devData[5]
        ,m_devData[6]
        );
        msg.data.property_post.params = property_params;
        aiot_dm_send(dm_handle, &msg);
    }

}
