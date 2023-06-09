#include "LGMQTT/PortLGMQTT.h"

#include "syslogger.h"

#include "Channel.h"

#include "MQTTClient.h"

//#include "MQTT/confighandler.h"
//#include "MQTT/sha1.hpp"
#include "LGMQTT/HMAC_MD5_API.h"

#include <time.h>
#include <stdio.h>

#include <exception>
#include "rapidjson/pointer.h"
#include "rapidjson/document.h"
#include "rapidjson/encodedstream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"

template<typename T>
T convert_from_rapidjson_value(rapidjson::Value & v, bool * ok = 0)
{
	if (v.IsNull() || v.GetType() != rapidjson::kNumberType) {
		if (ok) *ok = false;
		return 0;
	}
	if (ok) *ok = true;

	if (v.IsInt())      return (T)v.GetInt();
	if (v.IsInt64())    return (T)v.GetInt64();
	if (v.IsFloat())    return (T)v.GetFloat();
	if (v.IsDouble())   return (T)v.GetDouble();
	if (v.IsBool())     return (T)v.GetBool();

	if (ok) *ok = false;
	return 0;
}

class file_guard
{
public:
	file_guard(FILE * _fp) { fp_ = _fp; }
	~file_guard() { if (fp_) fclose(fp_); }
private:
	FILE * fp_;
};
namespace LGMQTT {

class PortLGImpl
{
public:
	int init(int32_t _chlid);
	int unInit();

	int  connect();
	void disconnect();

	MQTTClient client;
	MQTTClient_connectOptions connectOpt;

	std::string clientId;
	std::string host;
	std::string user;
	std::string pwd;
	std::string calpwd;
	std::string pubTopic;
	std::string subTopic;
};

int PortLGImpl::init(int32_t _chlid)
{
	FILE * fp = fopen("/root/comm/config/lg_mqtt_conf.json", "rb");
	if (!fp) {
		SysLogger::GetInstance()->LogError("read lg_mqtt_conf.json error.");
		return -1;
	}
	file_guard fg(fp);

	try {
		char readBuffer[1024] = { 0 };
		rapidjson::FileReadStream frs(fp, readBuffer, sizeof(readBuffer));
		rapidjson::AutoUTFInputStream<uint32_t, rapidjson::FileReadStream> auis(frs);  // 用 auis 包装 frs
		rapidjson::Document doc;

		if (doc.ParseStream< 0, rapidjson::UTF8<> >(auis).HasParseError())
		{
			SysLogger::GetInstance()->LogError("Json parse error(offset %d): %s",
				doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
			return -1;
		}

		rapidjson::Value * valueptr = rapidjson::Pointer("/list").Get(doc);
		if (!valueptr) {
			SysLogger::GetInstance()->LogWarn("Json parse error. no element.");
			return -1;
		}
		if (!valueptr->IsArray()) {
			SysLogger::GetInstance()->LogWarn("Json parse error. element error.");
			return -1;
		}

		for (size_t index = 0, arsize = valueptr->Size(); index < arsize; ++index)
		{
			char p[64] = {0};
			snprintf(p, sizeof(p) - 1, "/list/%d/channel_id", index);
			rapidjson::Value * itemptr = rapidjson::Pointer(p).Get(doc);
			if (!itemptr || _chlid != convert_from_rapidjson_value<int32_t>(*itemptr))
				continue;

			snprintf(p, sizeof(p) - 1, "/list/%d/client_id", index);
			itemptr = rapidjson::Pointer(p).Get(doc);
			if (itemptr) clientId = itemptr->GetString();

			snprintf(p, sizeof(p) - 1, "/list/%d/pub_topic", index);
			itemptr = rapidjson::Pointer(p).Get(doc);
			if (itemptr) pubTopic = itemptr->GetString();

			snprintf(p, sizeof(p) - 1, "/list/%d/user", index);
			itemptr = rapidjson::Pointer(p).Get(doc);
			if (itemptr) user = itemptr->GetString();

			snprintf(p, sizeof(p) - 1, "/list/%d/passwd", index);
			itemptr = rapidjson::Pointer(p).Get(doc);
			if (itemptr) pwd = itemptr->GetString();
		}
	}
	catch (std::exception & ex) {
		SysLogger::GetInstance()->LogError("%s", ex.what());
		return -1;
	}
	return 0;
}

int PortLGImpl::unInit()
{
	return 0;
}
string byteToHexStr(unsigned char byte_arr[], int arr_len)
{
    string  hexstr;//=new string();
    for (int i=0;i<arr_len;i++)
    {
    char hex1;
    char hex2;
    int value=byte_arr[i]; //直接将unsigned char赋值给整型的值，系统会正动强制转换
    int v1=value/16;
    int v2=value % 16;

    //将商转成字母
    if (v1>=0&&v1<=9)
    hex1=(char)(48+v1);
    else
    hex1=(char)(55+v1);

    //将余数转成字母
    if (v2>=0&&v2<=9)
    hex2=(char)(48+v2);
    else
    hex2=(char)(55+v2);

    //将字母连接成串
    hexstr=hexstr+hex1+hex2;
    }
    return hexstr;
}

int PortLGImpl::connect()
{
	time_t now = time(0);

	char now_str[64] = {0};
	if (sizeof(now) == 8)
		snprintf(now_str, sizeof(now_str) - 1, "%llu", (uint64_t)now);
	else
		snprintf(now_str, sizeof(now_str) - 1, "%u",   (uint32_t)now);

//	SHA1 checksum;
 //   checksum.update(std::string(now_str) + pwd);
 //   calpwd = std::string(now_str) + checksum.final();
    user =  "NO3979511536468992";
    ST_BYTE inputBuffer[14]={'E','L','1','2','3','4','5','6','7','8','9','A','B','C'};
    ST_BYTE passwordBuffer[16]={0xf5,0xaf,0xad,0x0e,0x77,0xcf,0x40,0x21,0x90,0x7f,0x95,0x52,0x51,0x37,0x05,0xfe};
    ST_BYTE outputBuffer[64];
    ST_INT MD5Len = HMAC_MD5_Hash( &inputBuffer[0], 14, &passwordBuffer[0], 16, &outputBuffer[0] );
    std::string strpsw ;
    calpwd = byteToHexStr(&outputBuffer[0],MD5Len);
   // memcpy(calpwd,strpsw,Strlen(strpsw));
 //   memcpy(calpwd,&outputBuffer[0],MD5Len);
 //   std::transform(calpwd.begin(), calpwd.end(), calpwd.begin(), ::toupper);

    connectOpt = MQTTClient_connectOptions_initializer;

	int rett = MQTTClient_create(&client, host.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rett != MQTTCLIENT_SUCCESS)
	{
        SysLogger::GetInstance()->LogError("MQTTClient_create FAILT %s %s %s %s %d",  host.c_str(),clientId.c_str(),now_str, calpwd.c_str(),rett);
	}

    connectOpt.cleansession = 1;
    connectOpt.reliable = 0;

    connectOpt.username = user.c_str();//NO3979511536468992
    connectOpt.password = calpwd.c_str();
    connectOpt.keepAliveInterval =200;
    int ret = MQTTClient_connect(client, &connectOpt);
    if (ret != MQTTCLIENT_SUCCESS) {
    	MQTTClient_destroy(&client);
    	SysLogger::GetInstance()->LogError("%s %s %s %s %d",  host.c_str(),clientId.c_str(),user.c_str(), calpwd.c_str(),ret);
    }
    return ret;
}

void PortLGImpl::disconnect()
{
	MQTTClient_disconnect(client, 10000L);
	MQTTClient_destroy(&client);
}

PortLGMQTT::PortLGMQTT(Channel *channel, PortInfo *portInfo):
PortBase(channel, portInfo),
impl_(new PortLGImpl)
{
}

PortLGMQTT::~PortLGMQTT()
{

}

ST_VOID PortLGMQTT::Init()
{
	if(m_Inited) return;

	int ret = impl_->init(m_pChannel->GetChannelInfo()->ChannelID);
	if (ret) return;

	char str[256] = {0};
	snprintf(str, sizeof(str) - 1, "tcp://%s:%d", m_pPortInfo->RemoteAddress, m_pPortInfo->RemotePort);

	impl_->host = str;

	m_pLogger->LogInfo("client_id : %s, pub_topic : %s, host: %s, user : %s, passwd : %s",
			impl_->clientId.c_str(), impl_->pubTopic.c_str(), impl_->host.c_str(), impl_->user.c_str(), impl_->pwd.c_str());

	m_Inited = true;
}

ST_VOID PortLGMQTT::Uninit()
{
	if(!m_Inited) return;

	impl_->unInit();

	m_Inited = false;
}

ST_VOID PortLGMQTT::Open()
{
    Locker locker(&(this->m_Mutex));
 //   if (m_IsOpened) Close();
  //  	return ;

    int ret = impl_->connect();
    if (ret != MQTTCLIENT_SUCCESS) {
		m_pLogger->LogError("MQTT Client connect error. code : (%d)", ret);
		Thread::SLEEP(1000);
        return ;
    }

    m_IsOpened = true;
}

ST_VOID PortLGMQTT::Close()
{
    Locker locker(&(this->m_Mutex));
    if (!m_IsOpened)
        return ;

    impl_->disconnect();

    m_IsOpened = false;
}

ST_BOOLEAN PortLGMQTT::Send(ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size)
{
    if(!m_IsOpened) Open();
    while (!m_IsOpened)
        Thread::SLEEP(100);

	Locker locker(&(this->m_Mutex));
    int ret = MQTTCLIENT_SUCCESS;

	MQTTClient_message pubMsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken deliveryToken;
    pubMsg.payload = pBuf; // message
    pubMsg.payloadlen = size ;//strlen(payload);
    pubMsg.qos = 2;         // qos level
    pubMsg.retained = 0;      // If we want to get message arrived time, set here to 1

  //  ret  = MQTTClient_create(&impl_->client, impl_->host.c_str(), impl_->clientId.c_str(),
  //                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
  //  if (ret != MQTTCLIENT_SUCCESS) {
  //      m_pLogger->LogError("MQTTClient MQTTClient_create Error. code : %d", ret);
   //     return false;
   // }

    ret = MQTTClient_publishMessage(impl_->client, impl_->pubTopic.c_str(), &pubMsg, &deliveryToken);

    if (ret != MQTTCLIENT_SUCCESS) {
        m_pLogger->LogError("MQTTClient PublishMessage Error. code : %d", ret);
        return false;
    }

 //   ret = MQTTClient_waitForCompletion(impl_->client, deliveryToken, 10000L);
 //   if (ret != MQTTCLIENT_SUCCESS) {
 //       m_pLogger->LogError("MQTTClient WaitForCompletion Error. code : %d", ret);
  //      return false;
 //   }

    Thread::SLEEP(100);

	return true;
}

ST_VOID PortLGMQTT::Recv()
{
	Thread::SLEEP(100);
    Locker locker(&(this->m_Mutex));

    int ret = MQTTClient_isConnected(impl_->client);
    if (!ret)
        impl_->disconnect();
    m_IsOpened = ret;
}

}; // namespace LGMQTT
