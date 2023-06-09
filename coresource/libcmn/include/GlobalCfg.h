#ifndef GLOBALCFG_H
#define GLOBALCFG_H

#include <vector>
#include "datatype.h"
#include "boost/shared_ptr.hpp"

#define MAXBUFFERLEN 2048

struct ProtocolTask *Memset  (struct ProtocolTask *dest, int c, size_t s);
struct ProtocolTask *Memcpy  (struct ProtocolTask *dest, ProtocolTask *src, size_t s);
struct ProtocolTask *Memmove (struct ProtocolTask *dest, ProtocolTask *src, size_t s);

#ifdef __cplusplus
extern "C"
{
#endif

using namespace std;

#define COMMUNICATIONDLL_EXPORTS

#ifdef COMMUNICATIONDLL_EXPORTS
#define COMMUNICATIONDLL_API __declspec(dllexport)
#else
#define COMMUNICATIONDLL_API __declspec(dllimport)
#endif

struct ProtocolTask_Result
{
	ProtocolTask_Result();
	ProtocolTask_Result(const ProtocolTask_Result &result);

	~ProtocolTask_Result();
	ST_INT          	resultCode;
	ST_CHAR             resultDesc[256];
	ST_INT              resultDataLen;
	ST_FLOAT            resultValue;
	ST_BYTE             resultData[1024];
};

struct ProtocolTask
{
	ProtocolTask();
	ProtocolTask(const ProtocolTask &task);
	~ProtocolTask();

	ProtocolTask& operator= (const ProtocolTask&);

	void Clear();

	ST_INT					channelId;
	ST_INT					deviceId;
	ST_UINT32				taskId;
	ST_CHAR					taskCmd[32];
	ST_INT					taskCmdCode;
	ST_INT					ignoreBack;
	ST_INT					taskTime;
	ST_INT					timeOut;
	ST_INT					taskAddr;
	ST_INT					taskAddr1;
	ST_CHAR     			taskStrAddr[64];
	ST_FLOAT				taskValue;
	ST_INT					taskParamLen;
	ST_BYTE 				taskParam[65536];
	ST_BOOLEAN              isTransfer;
	ST_INT					transChannelId;
	ST_INT					transDeviceId;
	ST_INT					isSupperTask;
	vector<ST_BYTE>			*taskList;
	ProtocolTask_Result		taskResult;
};

struct TaskNode
{
	TaskNode();
	~TaskNode();

	boost::shared_ptr<ProtocolTask> obj;
};


typedef ST_VOID (TXJ_STDCALL *ProtocolTaskBack )(const ProtocolTask &task);
typedef ST_VOID (TXJ_STDCALL *ShowMessageBack  )(const ST_CHAR *msg,ST_INT channelId, ST_INT deviceId);
typedef ST_VOID (TXJ_STDCALL *ShowMessageBackEx)(const ST_CHAR *msg,ST_INT channelId, ST_INT deviceId,ST_VOID *param);


class GlobalCfg
{
    enum ST_CMNMETHOD {MODULE, COMPONENT};
    private:
        GlobalCfg();
        virtual ~GlobalCfg();
    public:
        ST_VOID				SetWorkMethod(ST_CMNMETHOD wMethod);
        ST_CMNMETHOD		GetWorkMethod();
        static GlobalCfg	*GetInstance();
    protected:
    private:
        ST_CMNMETHOD		cmnMethod;
};

#ifdef __cplusplus
}
#endif

#endif // GLOBALCFG_H
