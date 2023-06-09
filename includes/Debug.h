
#ifndef _WEDO_DEBUG_H_
#define _WEDO_DEBUG_H_

#define NAMESPACE_WEDO_BEGIN
#define NAMESPACE_WEDO_END

// 启用支持协议类的宏, 如不需要注释它
#define WEDO_PROTOCOL_ENABLED

//#define wDebug		if (false) wedoDebug
//#define sDebug	if (false) wedoDebug (SysLogger::GetInstance()).noquote

#ifdef WEDO_PROTOCOL_ENABLED

#if defined(_MSC_VER)
#	include <Windows.h>
#	include "..\\include\\LineInfo.h"
#	include "..\\include\\CommField.h"
#   include "..\\include\\Channel.h"
#   include "..\\include\\ProtocolBase.h"
#	include "..\\include\\GenerallyVariableManager.h"
#endif // _MSC_VER

#ifdef __linux__
#	include "datatype.h"
#   include "Channel.h"
#   include "ProtocolBase.h"
#   include "Protocol.h"
#   include "DataCache.h"
#   include "Device.h"
#   include "Devices.h"
#   include "sysinifile.h"
#	include "ChannelConfig.h"

typedef ProtocolBase            CProtocolBase;

typedef ProtocolTask            PROTOCOLTASK;
typedef ProtocolTask_Result     PROTOCOLTASK_RESULT;

#endif // ! linux

#endif // WEDO_PROTOCOL_ENABLED

#include <stdio.h>
#include <time.h>
#include <list>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

typedef void (*wedoMsgHandler)(const char*);


NAMESPACE_WEDO_BEGIN

//----------CDebugStateKeeper类的声明-----------//
class CDebug;
class CDebugStateKeeper
{
public:
    CDebugStateKeeper (CDebug* d = NULL);
    ~CDebugStateKeeper ();
private:
    CDebug* classTemp;
    bool spaceTemp;
    int  flagsTemp;
}; // ! CDebugStateKeeper

class CDebug
{
    friend class CDebugStateKeeper;

	struct Stream {
#if defined(WEDO_PROTOCOL_ENABLED)
		Stream() : outptr(0), ref(1), space(true), message_output(true), flags(0) {}
	#if	defined(_MSC_VER)
		CProtocolBase* outptr;
	#else
		SysLogger* outptr;
	#endif
#else
		Stream() : ref(1), space(true), message_output(true), flags(0) {}
#endif
		std::ostringstream sBuf;
        int ref;
        bool space;
        bool message_output;

        enum FormatFlag {
            NoQuotes = 0x1
        };

        bool testFlag(FormatFlag flag) const { return ((flags & flag) ? true : false); }
        void setFlag(FormatFlag flag) { flags |= flag; }
        void unsetFlag(FormatFlag flag) { flags &= ~flag; }

        int flags;
    } *stream;

public:
#if !defined(WEDO_PROTOCOL_ENABLED)
	inline CDebug() : stream(new Stream()) {}
#elif defined(_MSC_VER)
	inline CDebug(CProtocolBase* p = 0) : stream(new Stream()) { stream->outptr = p; }
#else
	inline CDebug(SysLogger* p = 0) : stream(new Stream()) { stream->outptr = p; }
#endif
	inline CDebug(const CDebug &o) : stream(o.stream) { ++stream->ref; }
    inline CDebug& operator= (const CDebug &other)
		{ if (&other == this)return *this; delete stream; ++(other.stream->ref); stream = other.stream; return *this; }

	~CDebug () {
		if (! --stream->ref) {
			if(stream->message_output) {
#if defined(WEDO_PROTOCOL_ENABLED)
#if defined(_MSC_VER)
				if (stream->outptr) stream->outptr->OnShowMsg(stream->sBuf.str().c_str(), 0);
#else
                if (stream->outptr) stream->outptr->LogDebug(stream->sBuf.str().c_str());
#endif
				else
#endif
				wedo_message_output(stream->sBuf.str().c_str());
			}
			delete stream;
		}
	}

	// 注册CDebug类的输出函数
	static wedoMsgHandler wedoInstallMsgHandler(wedoMsgHandler h)
	{
		wedoMsgHandler old = handler;
		handler = h;
		return old;
	}

	inline CDebug &space() { stream->space = true; stream->sBuf << ' '; return *this; }
    inline CDebug &nospace() { stream->space = false; return *this; }
    inline CDebug &maybeSpace() { if (stream->space) stream->sBuf << ' '; return *this; }

    inline CDebug &quote() { stream->unsetFlag(Stream::NoQuotes); return *this; }
    inline CDebug &noquote() { stream->setFlag(Stream::NoQuotes); return *this; }
    inline CDebug &maybeQuote(char c = '"') { if (!(stream->testFlag(Stream::NoQuotes))) stream->sBuf << c; return *this; }

    inline CDebug &operator<<(bool t) { stream->sBuf << (t ? "true" : "false"); return maybeSpace(); }
    inline CDebug &operator<<(char t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(signed short t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(unsigned short t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(signed int t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(unsigned int t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(signed long t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(unsigned long t) { stream->sBuf << t; return maybeSpace(); }
#if !defined(_MSC_VER) || (_MSC_VER > 1200)
    inline CDebug &operator<<(signed long long t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(unsigned long long t) { stream->sBuf << t; return maybeSpace(); }
#endif
    inline CDebug &operator<<(float t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(double t) { stream->sBuf << t; return maybeSpace(); }
    inline CDebug &operator<<(const char* t) { maybeQuote(); stream->sBuf << t; maybeQuote(); return maybeSpace(); }
	inline CDebug &operator<<(const wchar_t* t) { maybeQuote(); stream->sBuf << t; maybeQuote(); return maybeSpace(); }
    inline CDebug &operator<<(const std::string & t) { maybeQuote(); stream->sBuf << t; maybeQuote(); return maybeSpace(); }
	inline CDebug &operator<<(const void* t) { stream->sBuf << t; return maybeSpace(); }

#ifdef WEDO_PROTOCOL_ENABLED

	CDebug &operator<<(unsigned char t)
	{
	    CDebugStateKeeper csk(this);
	    nospace();
	    stream->sBuf << "0x" << byteToHexString(t);
	    return maybeSpace();
    }

	static std::string byteToHexString (unsigned char byte)
	{
		static const char bit[] = "0123456789ABCDEF";
		std::string strHex;
		strHex += bit[byte / 16];
		strHex += bit[byte % 16];
		return strHex;
	}

	static std::string bytesToHexString (const unsigned char *bytes, int len)
	{
		std::string strHex;
		for (int i = 0; i < len; ++i)
			strHex += byteToHexString (bytes[i]) += ' ';
		return strHex;
	}
#endif // WEDO_PROTOCOL_ENABLED

private:
	static wedoMsgHandler handler;

	// 选择输出方式
	void wedo_message_output(const char* buf)
	{
		if (handler) {
			(*handler)(buf);
		} else {
		    printf ("%s\n", buf);
		}
	}
}; // ! class CDebug


//----------CDebugStateKeeper类的实现-----------//
CDebugStateKeeper::CDebugStateKeeper(CDebug* d)
            : classTemp(d), spaceTemp(0), flagsTemp(0)
{
    if (classTemp) {
        spaceTemp = classTemp->stream->space;
        flagsTemp = classTemp->stream->flags;
    }
}
CDebugStateKeeper::~CDebugStateKeeper ()
{
    if (classTemp) {
        classTemp->stream->space = spaceTemp;
        classTemp->stream->flags = flagsTemp;
    }
}

namespace std {

template<class T>
CDebug& operator<< (CDebug& debug, const vector<T> & v)
{
    CDebugStateKeeper csk(&debug);
    debug.noquote().nospace() << "std:vector(";
    for (int i = 0; i < v.size(); ++i) {
        CDebugStateKeeper cskf(&debug);
        if(i)debug << ',';
        debug.quote() << v.at(i);
    }
    debug << ')';
    return debug.space();
}

template<class T>
CDebug& operator<< (CDebug& debug, const list<T> & l)
{
    CDebugStateKeeper csk(&debug);
    debug.noquote().nospace() << "std:list(";
    typename list<T>::const_iterator i = l.begin();
    for (; i != l.end(); i++) {
        CDebugStateKeeper cskf(&debug);
        if(i != l.begin())
            debug << ',';
        debug.quote() << *i;
    }
    debug << ')';
    return debug.space();
}

}; // ! namespace std

CDebug& operator<< (CDebug& debug, const struct tm& t)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote().nospace() << "struct tm ("
		<< "tm_year: "   << t.tm_year
		<< ", tm_mon: "  << t.tm_mon
		<< ", tm_yday: " << t.tm_yday
		<< ", tm_wday: " << t.tm_wday
		<< ", tm_mday: " << t.tm_mday
		<< ", tm_hour: " << t.tm_hour
		<< ", tm_min: "  << t.tm_min
		<< ", tm_sec: "  << t.tm_sec
		<< ", tm_isdst: "<< t.tm_isdst
		<<")";
	return debug;
}

#if defined(WEDO_PROTOCOL_ENABLED)// && defined(_MSC_VER)

/*声明*/
CDebug& operator<< (CDebug& debug, const PROTOCOLTASK& pt);


/*实现*/

#if !defined(_MSC_VER)
template<class T>
CDebug& operator<< (CDebug& debug, List<T> & l)
{
    CDebugStateKeeper csk(&debug);
    debug.noquote().nospace() << "std:list(";
    for (int i =0; i < l.GetCount(); i++) {
        CDebugStateKeeper cskf(&debug);
        if(i != 0)
            debug << ',';
        debug.quote() << *(l.GetItem(i));
    }
    debug << ')';
    return debug.space();
}
#endif

#if !defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const ST_DUADDR& sd)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nST_DUADDR("
		<< "type: "		<< sd.type
		<< ", device: "	<< sd.device
		<< ", connect: "<< sd.connect
		<< ", addr: "	<< sd.addr
		<<")";
	return debug;
}
#endif

#if !defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const ST_Variable& sv)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nST_Variable("
		<< "apptype: "				<< sv.apptype
		<< ", area: "				<< sv.area
		<< ", coefficient: "		<< sv.coefficient
		<< ", datalen: "			<< sv.datalen
		<< ", decimal: "			<< sv.decimal
		<< ", eventtype: "			<< sv.eventtype
		<< ", filtersavemaxvalue: "	<< sv.filtersavemaxvalue
		<< ", filtersaveminvalue: "	<< sv.filtersaveminvalue
		<< ", filtersavetype: "		<< sv.filtersavetype
		<< ", filtersavevariable: "	<< sv.filtersavevariable
		<< ", fromItemaddr: "		<< sv.fromItemaddr
		<< ", fromconnect: "		<< sv.fromconnect
		<< ", fromobject: "			<< sv.fromobject
		<< ", fromtype: "			<< sv.fromtype
		<< ", id: "					<< sv.id
		<< ", locked: "				<< sv.locked
		<< ", maxvaliedvalue: "		<< sv.maxvaliedvalue
		<< ", minvaliedvalue: "		<< sv.minvaliedvalue
		<< ", name: "				<< sv.name
		<< ", offset: "				<< sv.offset
		<< ", overlimit: "			<< sv.overlimit
		<< ", overlimitstate: "		<< sv.overlimitstate
		<< ", prevalue: "			<< sv.prevalue
		<< ", storagedistance: "	<< sv.storagedistance
		<< ", type: "				<< sv.type
		<< ", wReserved1: "			<< sv.wReserved1
		<< ", wReserved2: "			<< sv.wReserved2
		<< ", wReserved3: "			<< sv.wReserved3
		<< ", zero: "				<< sv.zero
		<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const LineParams& lp)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nLineParams ("
	<< "area: "						<< lp.area
	<< ", maxResendTimes: " 		<< lp.maxResendTimes
	<< ", maxBreakTimes: "			<< lp.maxBreakTimes
	<< ", attribute: "				<< lp.attribute
	<< ", run: "					<< lp.run
	<< ", lineNo: "					<< lp.lineNo
	<< ", lineAddr: "				<< lp.lineAddr
	<< ", lineAddr1: "				<< lp.lineAddr1
	<< ", lineName: "				<< lp.lineName
	<< ", lineAddrEX: "				<< lp.lineAddrEX
	<< ", deviceseriatype: "		<< lp.deviceseriatype
	<< ", tablename: "				<< lp.tablename
	<< ", tablefilterfiled: "		<< lp.tablefilterfiled
	<< ", tablefilterfiledvalue: "  << lp.tablefilterfiledvalue
	<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const ChannelParam& cp)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nChannelParam ("
		<< "bDebug: "			<< cp.bDebug
		<< ", breakWarning: "	<< cp.breakWarning
		<< ", channeladdr: "	<< cp.channeladdr
		<< ", LineInterval: "	<< cp.LineInterval
		<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const CChannelBase& cb)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nCChannelBase ("
		<< "m_break: "							<< cb.m_break
		<< ", m_ChannelID: "					<< cb.m_ChannelID
		<< ", m_ChannelName: "					<< cb.m_ChannelName
		<< ", m_channelParam: "					<< cb.m_channelParam
		<< ", &m_pEngineBase(CEngineBase*): "	<< cb.m_pEngineBase
		<< ", m_SourceView: "					<< cb.m_SourceView
		<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const MyProtocolType& mpt)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nMyProtocolType ("
		<< "m_lpClassName: "				<< mpt.m_lpClassName
		<< ", m_lpDescription: "			<< mpt.m_lpDescription
		<< ", m_lpDllname: "				<< mpt.m_lpDllname
		<< ", m_pNext(MyProtocolType*): "	<< mpt.m_pNext
		<< ")";
	return debug;
}
#endif

CDebug& operator<< (CDebug& debug, const CProtocolBase& pb)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nCProtocolBase ("
#if defined(_MSC_VER)
		<< "m_continue: "						<< pb.m_continue
		<< ", m_CurTask: "						<< pb.m_CurTask
		<< ", pFirstType(MyProtocolType*): "	<< pb.pFirstType
#else
        << ", m_Buffer: "                       << CDebug::bytesToHexString(pb.m_Buffer, sizeof(pb.m_Buffer))
        << ", m_pPorts (PortBases*): "          << pb.m_pPorts
#endif
		<< ", m_pChannel(*): "	            	<< pb.m_pChannel
		<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const DataAreaItem& dai)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nDataAreaItem ("
		<< "addr: "			<< dai.addr
		<< ", beginBit: "	<< dai.beginBit
#if defined(_MSC_VER)
		<< ", codifyType: " << dai.codifyType
#else
		<< ", codeType: "   << dai.codeType
#endif
		<< ", coeficient: " << dai.coeficient
		<< ", dataLen: "	<< dai.dataLen
		<< ", dataType: "	<< dai.dataType
		<< ", endBit: "		<< dai.endBit
		<< ", id: "			<< dai.id
		<< ", itemName: "	<< dai.itemName
		<< ", reserver0: "	<< dai.reserver0
		<< ", reserver1: "	<< dai.reserver1
		<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const DeviceDataArea& dda)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nDeviceDataArea ("
		<< "addr: "						<< dda.addr
		<< ", areaName: "				<< dda.areaName
		<< ", dataUnitLen: "			<< dda.dataUnitLen
		<< ", itemCount:"				<< dda.itemCount
		<< ", items(DataAreaItem*): "	<< dda.items
		<< ", len: "					<< dda.len
		<< ", readCode: "				<< dda.readCode
		<< ", reserver0: "				<< dda.reserver0
		<< ", reserver1: "				<< dda.reserver1
		<< ", writeCode: "				<< dda.writeCode
		<< ")";
	return debug;
}

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const TransformItem& tfi)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nTransformItem ("
		<< "area: "				<< tfi.area
		<< ", coefficient: "	<< tfi.coefficient
		<< ", itemtype: "		<< tfi.itemtype
		<< ", modify: "			<< tfi.modify
		<< ", pointname: "		<< tfi.pointname
		<< ", prevalue: "		<< tfi.prevalue
		<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const TransformTable& tft)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nTransformTable ("
		<< "count: "						<< tft.count
		<< ", type: "						<< tft.type
		<< ", items(TransformItem*): "		<< tft.items
		<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const CLineInfo& lineInfo)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nCLineInfo ("
	<< "protocol(CProtocolBase*): "				<< lineInfo.protocol
	<< ", protocollib(HINSTANCE): "				<< lineInfo.protocollib
	<< ", m_lineParam: "						<< lineInfo.m_lineParam
	<< ", protocolName: "						<< lineInfo.protocolName
	<< ", fileName: "							<< lineInfo.fileName
	<< ", lineState: "							<< lineInfo.lineState
	<< ", breakTimes: "							<< lineInfo.breakTimes
	<< ", m_pDataAreas(DeviceDataArea*): "		<< lineInfo.m_pDataAreas
	<< ", m_DataAreasCount:  "					<< lineInfo.m_DataAreasCount
	<< ", m_TransformTables(TransformTable*): "	<< lineInfo.m_TransformTables
	<< ")";
	return debug;
}
#endif

CDebug& operator<< (CDebug& debug, const PROTOCOLTASK_RESULT& ptr)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nPROTOCOLTASK_RESULT ("
	<< "resultCode: " 		<< ptr.resultCode
#if defined(_MSC_VER)
	<< ", decription: "		<< ptr.decription
#else
    << ", resultDesc: "     << ptr.resultDesc
#endif
	<< ", resultDataLen: "	<< ptr.resultDataLen
	<< ", resultValue: "	<< ptr.resultValue
	<< ", resultData: "		<< CDebug::bytesToHexString (ptr.resultData, ptr.resultDataLen)
	<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const PROTOCOLTASK& pt)
{
    CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nPROTOCOLTASK ("
#if defined(_MSC_VER)
	<< "channel ID: " 					<< pt.channelID
	<< ", task ID: "					<< pt.taskID
	<< ", deviceNo: "					<< pt.deviceNo
	<< ", begin Channel: "				<< pt.beginChannel
	<< ", begin Node: "					<< pt.beginNode
	<< ", ignoreback: "					<< pt.ignoreback
	<< ", time: "						<< pt.time
	<< ", time out: "					<< pt.timeout
	<< ", param len: "					<< pt.paramlen
	<< ", task Param: "					<< CDebug::bytesToHexString (pt.taskParam, pt.paramlen)
	<< ", task addr0: "					<< pt.taskaddr0
	<< ", task addr1: "					<< pt.taskaddr1
	<< ", task value: "					<< pt.taskvalue
	<< ", allowoverdate: "				<< pt.allowoverdate
	<< ", result: "						<< pt.result
	<< ", taskEvent(HANDLE): "			<< pt.taskEvent
	<< ", ptaskPointer(PROTOCOLTASK*): "<< pt.ptaskPointer
#else
	<< "channel ID: "	 				<< pt.channelId
	<< ", task ID: "					<< pt.taskId
	<< ", deviceNo: "					<< pt.deviceId
	<< ", ignoreback: "			    	<< pt.ignoreBack
	<< ", task time: "					<< pt.taskTime
	<< ", time out: "					<< pt.timeOut
	<< ", task param len: "				<< pt.taskParamLen
	<< ", task Param: "					<< CDebug::bytesToHexString (pt.taskParam, pt.taskParamLen)
	<< ", task addr: "					<< pt.taskAddr
	<< ", task addr1: "					<< pt.taskAddr1
	<< ", task value: "					<< pt.taskValue
	<< ", task str addr: "				<< pt.taskStrAddr
	<< ", isTransfer: "					<< pt.isTransfer
	<< ", trans Channel Id: "			<< pt.transChannelId
	<< ", trans Device Id: "			<< pt.transDeviceId
	<< ", isSupperTask: "				<< pt.isSupperTask
	<< ", task result: "				<< pt.taskResult
	<< ", taskList(*): "				<< pt.taskList
#endif
	<< ", taskCmd: "					<< pt.taskCmd
	<< ", taskCmdCode: "				<< pt.taskCmdCode
	<< ")";
	return debug;
}

#if !defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const TRVariable& tv)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nTRVariable ("
	<< "Addr: " 			<< tv.Addr
	<< ", Coefficient: " 	<< tv.Coefficient
	<< ", PreValue: " 		<< tv.PreValue
	<< ", ItemType: " 		<< tv.ItemType
	<< ", Modify: " 		<< tv.Modify
	<< ")";
	return debug;
}
#endif

#if !defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, PortTask& pt)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nPortTask ("
	<< "DeviceAddr: "          << pt.DeviceAddr
	<< ", PortSrcAddr: "       << pt.PortSrcAddr
	<< ", PortDstAddr: "       << pt.PortDstAddr
	<< ", LocalChannelID: "    << pt.LocalChannelID
	<< ", TransmitChannelId: " << pt.TransmitChannelId
	<< ", DeviceId: "          << pt.DeviceId
	<< ", IsTransfer: "        << pt.IsTransfer
	<< ", KnowIPAddr: "        << pt.KnowIPAddr
	<< ", Buffer: "            << CDebug::bytesToHexString (pt.GetBuffer(), pt.GetBufferLen())
	<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const TransferTable& tt)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nTransferTable ("
	<< " TableType: " 		<< tt.TableType
	<< ", TableName: " 		<< tt.TableName
	<< ", m_pVarAddrs: " 	<< tt.m_pVarAddrs
	<< ")";
	return debug;
}
#endif

#if defined(_MSC_VER)
CDebug& operator<< (CDebug& debug, const Attribute& ab)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nAttribute ("
		<< "attribute: " << ab.attribute
 		<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const State& s)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nState ("
		<< "state: " << s.state
		<< ")";
	return debug;
}

CDebug& operator<< (CDebug& debug, const VariableObjectInfo& voi)
{
	CDebugStateKeeper csk(&debug);
	debug.noquote() << "\nVariableObjectInfo ("
		<< "access: "				<< voi.access
		<< ", apptype: "			<< voi.apptype
		<< ", area: "				<< voi.area
		<< ", attribute: "			<< voi.attribute
		<< ", coefficient: "		<< voi.coefficient
		<< ", datalen: "			<< voi.datalen
		<< ", decimal: "			<< voi.decimal
		<< ", eventtype: "			<< voi.eventtype
		<< ", filtersavemaxvalue: "	<< voi.filtersavemaxvalue
		<< ", filtersaveminvalue: "	<< voi.filtersaveminvalue
		<< ", filtersavetype: "		<< voi.filtersavetype
		<< ", filtersavevariable: "	<< voi.filtersavevariable
		<< ", fromconnect: "		<< voi.fromconnect
		<< ", fromItemaddr: "		<< voi.fromItemaddr
		<< ", fromobject: "			<< voi.fromobject
		<< ", fromtype: "			<< voi.fromtype
		<< ", id: "					<< voi.id
		<< ", locked: "				<< voi.locked
		<< ", maxvaliedvalue: "		<< voi.maxvaliedvalue
		<< ", minvaliedvalue: "		<< voi.minvaliedvalue
		<< ", name: "				<< voi.name
		<< ", offset: "				<< voi.offset
		<< ", overlimit: "			<< voi.overlimit
		<< ", overlimitstate: "		<< voi.overlimitstate
		<< ", prevalue: "			<< voi.prevalue
		<< ", state: "				<< voi.state
		<< ", storagedistance: "	<< voi.storagedistance
		<< ", type: "				<< voi.type
		<< ", userstate: "			<< voi.userstate
		<< ", zero: "				<< voi.zero
		<< ")";
	return debug;
}
#endif

#endif // WEDO_PROTOCOL_ENABLED

void outtofile(const char * msg)
{
	std::ofstream fout("debug.txt", std::fstream::app);
	fout << msg << "\r\n";
	fout.close();
}

wedoMsgHandler CDebug::handler = outtofile;

#if !defined(WEDO_PROTOCOL_ENABLED)
	CDebug wedoDebug() { return CDebug().noquote(); }
#elif defined(_MSC_VER)
	CDebug wedoDebug(CProtocolBase* ptr = 0) { return CDebug(ptr); }
#else
	CDebug wedoDebug(SysLogger* ptr = 0) { return CDebug(ptr); }
#endif // _MSC_VER


#endif // _WEDO_DEBUG_H_
