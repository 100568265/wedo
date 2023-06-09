
#include "MQTT/PortICBC.h"

#include "syslogger.h"

#include "Channel.h"

#include "MQTTClient.h"

#include "MQTT/confighandler.h"
#include "MQTT/sha1.hpp"

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

namespace MQTT {

class PortICBCImpl
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

int PortICBCImpl::init(int32_t _chlid)
{
	FILE * fp = fopen("/root/comm/config/icbc_mqtt_conf.json", "rb");
	if (!fp) {
		SysLogger::GetInstance()->LogError("read icbc_mqtt_conf.json error.");
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

int PortICBCImpl::unInit()
{
	return 0;
}

int PortICBCImpl::connect()
{
	time_t now = time(0);

	char now_str[64] = {0};
	if (sizeof(now) == 8)
		snprintf(now_str, sizeof(now_str) - 1, "%llu", (uint64_t)now);
	else
		snprintf(now_str, sizeof(now_str) - 1, "%u",   (uint32_t)now);

	SHA1 checksum;
    checksum.update(std::string(now_str) + pwd);
    calpwd = std::string(now_str) + checksum.final();
    std::transform(calpwd.begin(), calpwd.end(), calpwd.begin(), ::toupper);

    connectOpt = MQTTClient_connectOptions_initializer;

	int rett = MQTTClient_create(&client, host.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rett != MQTTCLIENT_SUCCESS)
	{
        SysLogger::GetInstance()->LogError("MQTTClient_create FAILT %s %s %s %s %d",  host.c_str(),clientId.c_str(),now_str, calpwd.c_str(),rett);
	}

    connectOpt.cleansession = 1;
    connectOpt.reliable = 0;

    connectOpt.username = user.c_str();
    connectOpt.password = calpwd.c_str();
    connectOpt.keepAliveInterval =200;
    int ret = MQTTClient_connect(client, &connectOpt);
    if (ret != MQTTCLIENT_SUCCESS) {
    	MQTTClient_destroy(&client);
    	SysLogger::GetInstance()->LogError("%s %s %s %s %d",  host.c_str(),clientId.c_str(),now_str, calpwd.c_str(),ret);
    }
    return ret;
}

void PortICBCImpl::disconnect()
{
	MQTTClient_disconnect(client, 10000L);
	MQTTClient_destroy(&client);
}

PortICBC::PortICBC(Channel *channel, PortInfo *portInfo):
PortBase(channel, portInfo),
impl_(new PortICBCImpl)
{
}

PortICBC::~PortICBC()
{

}

ST_VOID PortICBC::Init()
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

ST_VOID PortICBC::Uninit()
{
	if(!m_Inited) return;

	impl_->unInit();

	m_Inited = false;
}

ST_VOID PortICBC::Open()
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

ST_VOID PortICBC::Close()
{
    Locker locker(&(this->m_Mutex));
    if (!m_IsOpened)
        return ;

    impl_->disconnect();

    m_IsOpened = false;
}

ST_BOOLEAN PortICBC::Send(ST_UINT64 portAddr, char * pBuf, ST_UINT size)
{

}

ST_BOOLEAN PortICBC::Send(ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size)
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

ST_VOID PortICBC::Recv()
{
	Thread::SLEEP(100);
 //   Locker locker(&(this->m_Mutex));
//
 //   int ret = MQTTClient_isConnected(impl_->client);
 //   if (!ret)
 //       impl_->disconnect();
 //   m_IsOpened = ret;
}

}; // namespace MQTT
