//#include "stdafx.h"
#include "GlobalCfg.h"

#include "sysstring.h"
#include "sysmalloc.h"

#include <time.h>
#include <string.h>

ProtocolTask *Memset  (ProtocolTask *dest, int c, size_t s)
{
	if (dest) dest->Clear();
	return dest;
}

ProtocolTask *Memcpy  (ProtocolTask *dest, ProtocolTask *src, size_t s)
{
	if (dest && src) *dest = *src;
	return dest;
}

ProtocolTask *Memmove (ProtocolTask *dest, ProtocolTask *src, size_t s)
{
	if (dest && src) *dest = *src;
	return dest;
}

ProtocolTask_Result::ProtocolTask_Result()
{}

ProtocolTask_Result::~ProtocolTask_Result()
{}

ProtocolTask_Result::ProtocolTask_Result(const ProtocolTask_Result &result)
{
	memcpy (this, &result, sizeof(ProtocolTask_Result));
	// resultCode=result.resultCode;
	// Strcpy(resultDesc,result.resultDesc);
	// resultDataLen=result.resultDataLen;
	// resultValue=result.resultValue;
	// Memcpy(resultData,result.resultData,1024);
}

ProtocolTask::ProtocolTask()
{
	taskId         = time_t(NULL);
	channelId      = -1;
	deviceId       = -1;
	taskCmd[0]     = '\0';
	taskCmdCode    = -1;
	ignoreBack     = -1;
	taskTime       = -1;
	timeOut        = -1;
	taskAddr       = -1;
	taskAddr1      = -1;
	taskStrAddr[0] = '\0';
	taskValue      = -1;
	taskParamLen   = 0;
	isTransfer     = false;
	transChannelId = -1;
	transDeviceId  = -1;
	isSupperTask   = 0;
	taskList       = NULL;
}

ProtocolTask::ProtocolTask(const ProtocolTask &task):
taskList(0)
{ *this = task; }

ProtocolTask::~ProtocolTask()
{
	if(taskList != NULL) {
		delete taskList;
		taskList = NULL;
	}
}

ProtocolTask& ProtocolTask::operator= (const ProtocolTask& task)
{
	if (&task == this)
		return *this;

	taskValue      = task.taskValue;
	transChannelId = task.transChannelId;
	transDeviceId  = task.transDeviceId;
	isTransfer     = task.isTransfer;
	channelId      = task.channelId;
	deviceId       = task.deviceId;
	taskId         = time_t(NULL);
	taskCmdCode    = task.taskCmdCode;
	ignoreBack     = task.ignoreBack;
	taskTime       = task.taskTime;
	timeOut        = task.timeOut;
	taskAddr       = task.taskAddr;
	taskAddr1      = task.taskAddr1;
	isSupperTask   = task.isSupperTask;
	taskResult     = task.taskResult;
	taskParamLen   = task.taskParamLen;
	Strcpy(taskCmd, task.taskCmd);
	Strcpy(taskStrAddr, task.taskStrAddr);
	Memcpy(taskParam, task.taskParam, task.taskParamLen);

	if (taskList) {
		delete taskList;
		taskList = NULL;
	}
	if (task.taskList)
		taskList = new vector<ST_BYTE>(*task.taskList);

	return *this;
}

void ProtocolTask::Clear()
{
	taskId=time_t(NULL);
	channelId=-1;
	deviceId=-1;
	taskCmd[0]='\0';
	taskCmdCode=-1;
	ignoreBack=-1;
	taskTime=-1;
	timeOut=-1;
	taskAddr=-1;
	taskAddr1=-1;
	taskStrAddr[0]='\0';
	taskValue=-1;
	taskParamLen=0;
	isTransfer=false;
	transChannelId=-1;
	transDeviceId=-1;
	isSupperTask=0;

	if (taskList) {
		delete taskList;
		taskList = NULL;
	}
}

TaskNode::TaskNode()
{
}

TaskNode::~TaskNode()
{
}

GlobalCfg::GlobalCfg()
{
    cmnMethod=MODULE;
}

GlobalCfg::~GlobalCfg()
{

}

ST_VOID GlobalCfg::SetWorkMethod(GlobalCfg::ST_CMNMETHOD wMethod)
{
    cmnMethod=wMethod;
}

GlobalCfg::ST_CMNMETHOD GlobalCfg::GetWorkMethod()
{
    return cmnMethod;
}

GlobalCfg *GlobalCfg::GetInstance()
{
    static GlobalCfg cfg;
    return &cfg;
}
