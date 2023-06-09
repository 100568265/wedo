#ifndef ACTUALVALUESERVER_H
#define ACTUALVALUESERVER_H
#include "ChannelConfig.h"
#include "PortBase.h"
#include "PortTcpServerBase.h"

#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include "datatype.h" //
#include "rtobjecttree.h" //


class ActualValueServer
{
public:
	ActualValueServer();
	virtual ~ActualValueServer();
	ST_VOID Init();
	ST_VOID Uninit();
	ST_VOID	Work();
	ST_VOID SendMessage(const ST_CHAR *msg, ST_INT dataLen, ST_INT deviceId);
	ST_VOID protocolMsg();
#ifdef _WIN32
	static ST_UINT32 __stdcall	ReceiveProc(ST_VOID *param);
#else
    static ST_VOID				*ReceiveProc(ST_VOID *param);
#endif

    static ST_VOID				*SendProc(ST_VOID *param);
private:
	PortTcpServerBase *m_port;
	PortInfo m_PortInfo;
	Thread	m_thread;
	Thread  m_sendThread;
	ST_BOOLEAN m_init;
	ST_BOOLEAN m_working;
};


#endif //ACTUALVALUESERVER_H
