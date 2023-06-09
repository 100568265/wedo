#ifndef PROTOCOLBASE_H
#define PROTOCOLBASE_H

#include "datatype.h"
#include "systhread.h"
#include "sysmutex.h"
#include "sysmalloc.h"
#include "sysqueue.h"
#include "PortTask.h"
#include "PortBases.h"
#include "PortBuffer.h"
#include "DataCache.h"
#include "CmnRtInterface.h"

class Channel;
class Device;
class Devices;
class EngineBase;

class SysLogger;

#ifdef _WIN32
template class __declspec(dllexport) Queue<TaskNode>;
class __declspec(dllexport) Mutex;
class COMMUNICATIONDLL_API ProtocolBase
#else
class ProtocolBase
#endif
{
public:
    ProtocolBase();
    virtual ~ProtocolBase();
    virtual ST_VOID		Init();
    virtual ST_VOID		Uninit();
	virtual ST_VOID		Start();
    virtual ST_VOID		Stop();
    virtual ST_VOID		OnRead(ST_BYTE *pBuf,ST_INT &reviced)=0;
	virtual ST_BOOLEAN  OnSend()=0;
	virtual ST_BOOLEAN  OnProcess(ST_BYTE *pBuf,ST_INT len);
	virtual ST_BOOLEAN  IsContinue();
	virtual ST_VOID		ReSendFailed(){};
	virtual ST_BOOLEAN  OnRead(PortTask *task);
	virtual ST_VOID		OnConnect(ST_INT port,ST_UINT64 portAddr);
	virtual ST_VOID		OnDisconnect(ST_INT port,ST_UINT64 portAddr);
	virtual ST_BOOLEAN	IsSupportEngine(ST_INT engineType)=0; ///< 重载此函数, 用以判断engineType是否为可支持类型, 支持返回true.
	ST_VOID				OnLinkBreak();
	ST_VOID				UpdateDeiveState(int state);
    ST_BOOLEAN			HasTask();
    ST_BOOLEAN			GetTask(ProtocolTask *task, ST_BOOLEAN pop = true);
    ST_VOID				OnTask(ProtocolTask *task);
	ST_VOID				OnTask(ProtocolTask *task,ST_BYTE *data,ST_INT dataLen);
    ST_VOID				OnTaskBack(ProtocolTask *task);
    ST_VOID				SetDevice(Device *device);
	Device				*GetDevice();
	ST_VOID				ShowMessage(const ST_CHAR *msg);
    ST_VOID				ShowSendFrame(ST_BYTE *buf, ST_INT len);
    ST_VOID				ShowRecvFrame(ST_BYTE *buf, ST_INT len);
	SysLogger			*GetLogger();
	ST_BOOLEAN			IsOpened();
	ST_BOOLEAN			ReSend();
	ST_VOID				Continue();


protected:
    ST_VOID				Open();
    ST_VOID				Close();
	ST_VOID				Close(ST_UINT64 port);
    ST_BOOLEAN			Send(ST_BYTE *pBuf,ST_INT len,ST_UINT64 dstAddr=-1);
	ST_BOOLEAN          Transfer(ProtocolTask *task);
	DataCache			*GetCurPort(ST_UINT64 port=-1);
	PortBase			*GetMainPort();
	PortBase			*GetSlavPort();
	Devices				*GetDevices();
	ST_INT              GetDeviceId();
public:
	ST_BYTE				m_Buffer[MAXBUFFERLEN];
	Channel             *m_pChannel;
	PortBases           *m_pPorts;
protected:
	Mutex               m_Mutex;
    Device              *m_pDevice;
    SysLogger           *m_pLogger;
	Queue<TaskNode>     m_ProtocolTasks;
	ST_UINT64           m_DestAddr;
	ST_BYTE*			m_BakBuffer;
	ST_INT				m_BakSendLength;
	ST_INT				m_MaxSendLength;
	ST_CHAR				m_Msg[2048];
	ST_BOOLEAN			m_continue;
	ST_BOOLEAN			m_bBreak;
};

typedef ProtocolBase    CProtocolBase;
typedef ProtocolBase* (*CreateProtocol)();

#endif // PROTOCOLBASE_H
