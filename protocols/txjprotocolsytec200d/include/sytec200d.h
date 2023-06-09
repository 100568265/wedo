
#ifndef _SYTEC200D_H_
#define _SYTEC200D_H_

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class SYTEC200D : public Protocol
{
public:
	SYTEC200D();
	virtual ~SYTEC200D();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
protected:

	void      SendEx (ST_BYTE * data);
	ST_BYTE   GetCtrlCode (ST_BYTE code, bool fcv_flag);
	void      Call1stData ();
	void      Call2ndData ();
	void      CallTotal   ();
	void	  CallYMData  ();

	void   AnalysisFor2ndData (ST_BYTE * data);
	void   AnalysisForYXData  (ST_BYTE * data);
	void   AnalysisForYMData  (ST_BYTE * data);
	void   AnalysisForSOE     (ST_BYTE * data);

	void   SetTime ();

	void   YKSelect (const ProtocolTask&);
	void   YKExecut (const ProtocolTask&);
	void   YKCancel (const ProtocolTask&);

	time_t     m_lasttime;
	ST_BOOLEAN m_bIsSendSuc;
	ST_BOOLEAN m_has1stData;
	ST_BOOLEAN m_fcb_flag;

	ST_BOOLEAN m_bTask;
	ProtocolTask m_curTask;
private:

	ST_BYTE  m_totalcallcount;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API SYTEC200D* CreateInstace();
#else
	SYTEC200D* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif //_SYTEC200D_H_