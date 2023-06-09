
#include "RtValueTransfer.h"

#include "rtbase.h"

#include "websocket.h"
#include "wedo/thread/this_thread.h"
#include "wedo/thread/thread.h"

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>

#include <map>
#include <set>
#include <string>

#include "ArduinoJson.h"
#include "mongoose.h"

inline static int is_websocket (const struct mg_connection * nc)
{
    return nc->flags & MG_F_IS_WEBSOCKET;
}

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

struct _DuID
{
	 _DuID() : Type(0), ChlId(0), DevId(0) {}

    inline bool operator== (const _DuID& other) const
	{
		return Type == other.Type && ChlId == other.ChlId && DevId == other.DevId;
	}
    inline bool operator!= (const _DuID& other) const
		{ return ! (*this == other); }

    inline bool operator< (const _DuID& other) const
		{ return Type < other.Type && ChlId < other.ChlId && DevId < other.DevId; }
    inline bool operator> (const _DuID& other) const
		{ return Type > other.Type && ChlId > other.ChlId && DevId > other.DevId; }
    inline bool operator>=(const _DuID& other) const
		{ return !(*this < other); }
    inline bool operator<=(const _DuID& other) const
		{ return !(*this > other); }

	int32_t Type ;
	int32_t ChlId;
	int32_t DevId;
};

struct _RtValueTransferPrivate
{
	typedef std::map<_DuID, std::set<int32_t> > set_type;

	struct mg_connection * _nc;
	boost::scoped_ptr<wedo::ws_server> _server;
	set_type _set;
	wedo::thread _thr;
	bool _is_thr_working;

	_RtValueTransferPrivate() :
        _nc(NULL),
        _thr(boost::bind(&_RtValueTransferPrivate::Poll, this)),
        _is_thr_working(true)
        {
        }

	void Send (const void * buf, int len)
    {
        if (_nc) //mg_send(_nc, buf, len);
            mg_send_websocket_frame(_nc, WEBSOCKET_OP_TEXT, buf, len);
    }
	void Recv (void * buf, int len);
	void Poll ();

	int  ParseFrame (const char * buf);
	void ReportErr (const char * msg, uint32_t no);
	int  PrintTo ();
	int  FindTheAddToSet(_DuID& duid);

	void EvHandler(struct mg_connection * nc, int ev, void *ev_data);
};

void _RtValueTransferPrivate::EvHandler(struct mg_connection * nc, int ev, void *ev_data)
{
	switch (ev) {

	    case MG_EV_POLL: {
	    } break;
	    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
	    	/// New websocket connection.
            if (_nc && _nc != nc)
                _nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            _nc = nc;
	    } break;
	    case MG_EV_WEBSOCKET_FRAME: {
	        struct websocket_message * wm = (struct websocket_message *) ev_data;
	        wm->data[wm->size] = 0;
	        /// New websocket message.
	        this->Recv(wm->data, wm->size);

	    } break;
	    case MG_EV_CLOSE: {
	        /// Disconnect.
	    	if (_nc == nc)
	    		_nc = NULL;
           _set.clear();
	    } break;
	}
}

void _RtValueTransferPrivate::Recv(void * buf, int len)
{
	if (len > 512) {
		return;
	}

	this->ParseFrame((const char*)buf);
}

void _RtValueTransferPrivate::Poll()
{
    while (_is_thr_working) {
        PrintTo();
        wedo::this_thread::msleep(1000);
	}
}

int _RtValueTransferPrivate::PrintTo()
{
	if (_set.empty())
		return -1;

	for (set_type::iterator set_it = _set.begin();
		set_it != _set.end(); ++set_it)
	{
		ST_DUADDR duaddr;
		duaddr.type    = set_it->first.Type ;
		duaddr.connect = set_it->first.ChlId;
		duaddr.device  = set_it->first.DevId;

		DynamicJsonBuffer jsonbuf;
		JsonObject&root = jsonbuf.createObject();
		root["Func" ] = "RtValue";
		root["Cmd"  ] = "SendDataNSQ";
		JsonObject&data = root.createNestedObject("Data");
		data["Type" ] = duaddr.type;
		data["ChlId"] = duaddr.connect;
		data["DevId"] = duaddr.device;

		JsonArray&values = data.createNestedArray("Values");

		std::set<int32_t> & ref = set_it->second;
		for (std::set<int32_t>::iterator it = ref.begin();
			it != ref.end(); ++it)
		{
			duaddr.addr = *it;
			ST_VARIANT var;
			if (Rt_GetNodeValueByAddr(&duaddr, &var) < 0)
				continue;

			JsonArray&value = values.createNestedArray();
			value.add(duaddr.addr);
			value.add(variant_trans_handler<double>(var));
		}
		std::string outstr;
		root.printTo(outstr);
		this->Send(outstr.c_str(), outstr.size());
		wedo::this_thread::msleep(100);
	}
	return 0;
}

void _RtValueTransferPrivate::ReportErr (const char * msg, uint32_t no)
{
	StaticJsonBuffer<256> jsonbuf;
	JsonObject& root = jsonbuf.createObject();
	root["Func"] = "RtValue";
	root["Cmd" ] = "Report" ;
	root["IsOK"] = "false"  ;
	if (msg)
		root[ "Msg" ] = msg;
	else
		root["Errno"] = no ;

	char buf[256] = {0};
	int len = root.printTo (buf, 256);
	this->Send(buf, len);
}

int _RtValueTransferPrivate::ParseFrame(const char * buf)
{
	std::string str = buf;
	StaticJsonBuffer<512> jsonbuf;
	JsonObject& root = jsonbuf.parseObject(str);
	if (!root.success())
	{
		// error Decoding JSON.
		ReportErr("Json parse error.", -1);
		return -1;
	}

	const char * func = root["Func"];
	if (func && std::string(func) != "RtValue")
	{
		// func key not match.
		ReportErr("Function does not match.", -1);
		return -2;
	}
	const char * cmd = root["Cmd"];

	if (cmd && std::string(cmd) == "Register")
	{
		JsonObject& target = root["Target"];
		if (!target.success() || !root.is<JsonObject&>("Target"))
		{
		    ReportErr("Json parse error.", -1);
		    return -4;
		}
		_DuID duid;
		duid.Type  = target["Type" ];
		duid.ChlId = target["ChlId"];
		duid.DevId = target["DevId"];
		FindTheAddToSet (duid);
	}
	else if (cmd &&  std::string(cmd) == "UnRegister")
	{
		JsonObject& target = root["Target"];
		if (!target.success() || !root.is<JsonObject&>("Target"))
		{
		    ReportErr("Json parse error.", -1);
		    return -5;
		}
		_DuID duid;
		duid.Type  = target["Type" ];
		duid.ChlId = target["ChlId"];
		duid.DevId = target["DevId"];

        if (duid.Type == -657676 && duid.ChlId == -657676 && duid.DevId ==  -657676)
            _set.clear();
        else
            _set.erase(duid);
	}
	else
	{
		ReportErr("Can not find command code.", -1);
		return -6;
	}

	return 0;
}

int _RtValueTransferPrivate::FindTheAddToSet(_DuID & duid)
{
	ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
	int32_t devcount = Rt_GetNameNodeCount(0);
	for (int32_t devindex = 0; devindex < devcount; ++devindex)
	{
		*(int32_t*)devname = 0;
		if (Rt_GetNodeName(0, devindex, devname) < 0)
			continue;

		int32_t varcount = Rt_GetNameNodeCount(devname);
		for (int32_t varindex = 0; varindex < varcount; ++varindex)
		{
			*(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

			if (Rt_GetNodeName(devname, varindex, ditname) < 0)
				continue;

			strcpy (fullname, devname); strcat (fullname, ".");
			strcat (fullname, ditname); strcat (fullname, ".value");

			ST_DUADDR duaddr;
			Rt_GetNodeAddr(fullname, &duaddr);
			if (duid.Type  == duaddr.type && duid.ChlId == duaddr.connect)
			{
				if (duid.DevId == -657676 || duid.DevId == duaddr.device)
					_set[duid].insert(duaddr.addr);
			}
		}
	}
	return 0;
}

RtValueTransfer::RtValueTransfer():
_p(new _RtValueTransferPrivate())
{

}

RtValueTransfer::~RtValueTransfer()
{

}

void RtValueTransfer::Init(const char * address)
{
	_p->_server.reset(new wedo::ws_server(address, boost::bind(&_RtValueTransferPrivate::EvHandler, _p, _1, _2, _3)));
}

void RtValueTransfer::Start()
{
	if (_p->_server)
		_p->_server->start();
    _p->_is_thr_working = true;
    _p->_thr.start();
}

void RtValueTransfer::Stop()
{
	_p->_server.reset();
	_p->_is_thr_working = false;
	_p->_thr.join();
}

RtValueTransfer& RtValueTransfer::Instance()
{
	static RtValueTransfer rt;
	return rt;
}
