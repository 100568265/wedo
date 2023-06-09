
#include "transjsonelec.h"

#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/pointer.h"

#include "systhread.h"
#include "TransTable.h"
#include "Device.h"

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

_PROTOCOL_INTERFACE_IMPLEMENT_(TransJsonelec)

TransJsonelec::TransJsonelec()
{}

TransJsonelec::~TransJsonelec()
{}

void TransJsonelec::Init()
{}

void TransJsonelec::Uninit()
{}

void TransJsonelec::OnRead(ST_BYTE * pbuf, ST_INT & readed)
{
    readed = 0;

    Thread::SLEEP(100);
}

ST_BOOLEAN TransJsonelec::OnSend()
{
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables || trantables->GetCount() <= 0) {
        this->ShowMessage("No transfer table.");
        Thread::SLEEP(100);
		return false;
	}

    rapidjson::Document jsondoc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc = jsondoc.GetAllocator();

    jsondoc.AddMember("func", "initiative_trans", alloc);

	for (int iter = 0; iter < trantables->GetCount(); ++iter)
	{
        TransferTable * table = 0;
        List<ST_DUADDR> * list = 0;

        if ((table = trantables->GetItem(iter)) == NULL) {
		    continue;
	    }
	    if ((list = table->m_pVarAddrs) == NULL) {
	    	continue;
	    }

        time_t nowt = time(0);
        struct tm nowtm;

        if (nowt == (time_t)-1 || localtime_r(&nowt, &nowtm) == NULL)
        {
            this->ShowMessage("Unable to get the current time.");
            Thread::SLEEP(100);
            return true;
        }

        char datestr [128] = {0};
        snprintf(datestr, sizeof(datestr)-1, "%04d-%02d-%02d %02d:%02d:%02d",
            nowtm.tm_year + 1900, nowtm.tm_mon + 1, nowtm.tm_mday, nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec);

        rapidjson::Pointer("/time").Set(jsondoc, datestr);

        char snstr [128] = {0};
        snprintf(snstr, sizeof(snstr)-1, "VAGARY-ELEC-%d", table->typeId());

        rapidjson::Pointer("/devsn").Set(jsondoc, snstr);

        rapidjson::Value vars(rapidjson::kArrayType);

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

            rapidjson::Value pair(rapidjson::kArrayType);
            pair.PushBack(rapidjson::Value(it).Move(), alloc);
            pair.PushBack(rapidjson::Value(varTemp).Move(), alloc);

            vars.PushBack(pair.Move(), alloc);
        }

        rapidjson::Pointer("/vars").Set(jsondoc, vars);

        rapidjson::StringBuffer buffer(0, 1024);
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        jsondoc.Accept(writer);

        this->Send((ST_BYTE*)buffer.GetString(), buffer.GetSize() + 1);
        this->ShowMessage(buffer.GetString());

        Thread::SLEEP(800);
	}

    return true;
}

ST_BOOLEAN TransJsonelec::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
    return true;
}

ST_BOOLEAN TransJsonelec::IsSupportEngine(ST_INT engineType)
{
	return 0 == engineType; // EngineType == Fulling;
}

