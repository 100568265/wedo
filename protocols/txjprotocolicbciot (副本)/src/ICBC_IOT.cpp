
#include "ICBC_IOT.h"

#include <stdio.h>

#include "rapidjson/writer.h"
#include "rapidjson/pointer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#include "TransTable.h"
#include "Device.h"
#include "Channel.h"

template<typename T>
inline T variant_trans_handler (const ST_VARIANT& var)
{
	switch(var.vt) {
		case VALType_SByte  : return (T)var.cVal ;
		case VALType_Byte   : return (T)var.bVal ;
		case VALType_Int32  : return (T)var.iVal ;
		case VALType_UInt32 : return (T)var.uiVal;
		case VALType_Int64  : return (T)var.lVal ;
		case VALType_UInt64 : return (T)var.ulVal;
		case VALType_Float  : return (T)var.fVal ;
		case VALType_Double : return (T)var.dtVal;
	}
	return 0;
}


Protocol* CreateInstace()
{
	return new ICBC_IOT();
}

ICBC_IOT::ICBC_IOT():
index_(0)
{

}

ICBC_IOT::~ICBC_IOT()
{

}

void ICBC_IOT::Init()
{
	index_ = 0;
}

void ICBC_IOT::Uninit()
{
	index_ = 0;
}

void ICBC_IOT::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
    readed = 0;
    Thread::SLEEP(100);
}

ST_BOOLEAN ICBC_IOT::OnSend()
{
    while(!this->IsOpened())
    {
        this->ShowMessage("Open.");
        this->Open();
        Thread::SLEEP(2000);
    }
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables || trantables->GetCount() <= 0) {
        this->ShowMessage("No transfer table.");
        Thread::SLEEP(100);
		return false;
	}
	index_ = ++index_ % trantables->GetCount();

	TransferTable * table = 0;
    List<ST_DUADDR> * list = 0;

    if ((table = trantables->GetItem(index_)) == NULL) {
		return false;
	}
	if ((list = table->m_pVarAddrs) == NULL) {
		return false;
	}

	time_t now_t = time(0);
	struct tm now_tm;
	if (now_t == (time_t)-1 || localtime_r(&now_t, &now_tm) == NULL)
    {
        this->ShowMessage("Unable to get the current time.");
        Thread::SLEEP(100);
        return true;
    }

	char now_str[32] = { 0 };
	strftime(now_str, sizeof now_str, "%F %T", &now_tm);

	rapidjson::Document jsondoc(rapidjson::kObjectType);
	rapidjson::Document::AllocatorType & alloc = jsondoc.GetAllocator();

	jsondoc.AddMember("corpID", "001", alloc);

	char sn_str[64] = {0};
	snprintf(sn_str, sizeof(sn_str) - 1, "VAGARY925D-M5Y%d", table->typeId() + 2);
	jsondoc.AddMember("strTESN", rapidjson::StringRef(sn_str), alloc);
	jsondoc.AddMember("deviceType", "01", alloc);
	jsondoc.AddMember("deviceSubType", "0001", alloc);
	jsondoc.AddMember("protocolNo", "13", alloc);
	jsondoc.AddMember("length", 0, alloc);
    jsondoc.AddMember("sTimeStamp", rapidjson::StringRef(now_str), alloc);

	rapidjson::Value data(rapidjson::kObjectType);

	data.AddMember("READ_TIME", (int64_t)now_t, alloc);

	int interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	data.AddMember("FREQUENCY", interval / 60000, alloc);

	for (int32_t it = 0; it < list->GetCount(); ++it)
	{
	    ST_DUADDR  *duaddr = list->GetItem(it);
        TRVariable *TRVar  = table->GetTRVariable(duaddr);
        ST_VARIANT stvar;
        ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
        if (nR < 0) stvar.ulVal = 0;
        double varTemp = variant_trans_handler<double>(stvar);

		if (TRVar) {
            if (TRVar->Coefficient == 0) TRVar->Coefficient = 1;
            varTemp *= TRVar->Coefficient;
		}

		char key_str[32] = {0};
		snprintf(key_str, sizeof(key_str) - 1, "DATA%d", it + 1);
		rapidjson::Value name; name.SetString(key_str, strlen(key_str), alloc);
		data.AddMember(name, varTemp, alloc);
	}

	rapidjson::Pointer("/DATA").Set(jsondoc, data);

	rapidjson::StringBuffer buffer(0, 1024);
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	jsondoc.Accept(writer);

	std::string json = buffer.GetString();


	size_t pos = json.find("\"length\":0");
	if (pos != std::string::npos) {
		size_t p = json.find("\"DATA\"");
		while (json[p++] != '{')
			continue;
		int len = 0;
		while (json[p++] != '}')
			++len;

		char len_str[32] = { 0 };
		snprintf(len_str, sizeof len_str, "\"length\":%d", len);
		json.replace(pos, strlen("\"length\":0"), len_str);
	}

	this->Send((uint8_t*)json.c_str(), json.size());
	this->ShowMessage(json.c_str());

    Thread::SLEEP(100);
    this->Close();

	return true;
}

ST_BOOLEAN ICBC_IOT::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	return true;
}

ST_BOOLEAN ICBC_IOT::IsSupportEngine(ST_INT engineType)
{
	return 0 == engineType; // EngineType == Fulling;
}
