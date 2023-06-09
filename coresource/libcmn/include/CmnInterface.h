#ifndef CMNINTERFACE_H
#define CMNINTERFACE_H

#include "datatype.h"
#include "rtbase.h"
#include "Communication.h"

#ifdef __cplusplus
extern "C"
{
#endif


extern Communication g_Cmn;

#ifdef _WIN32
COMMUNICATIONDLL_API ST_VOID CmnInit();
COMMUNICATIONDLL_API ST_VOID CmnUnInit();
COMMUNICATIONDLL_API ST_VOID CmnStartup();
COMMUNICATIONDLL_API ST_VOID CmnStop();
COMMUNICATIONDLL_API ST_VOID CmnExcCmd(ProtocolTask task,ST_BOOLEAN ignorePort);
COMMUNICATIONDLL_API ST_VOID CmnRegistTaskBack(ProtocolTaskBack func);
COMMUNICATIONDLL_API ST_VOID CmnRegistMessageBack(ShowMessageBack func);
COMMUNICATIONDLL_API ST_VOID CmnRegistMessageBackEx(ShowMessageBackEx func,ST_VOID *callObj);
COMMUNICATIONDLL_API ST_VOID CmnRegistSourceView(int channelId,ST_BOOLEAN showView);

COMMUNICATIONDLL_API ST_INT  CmnAddChannel(ChannelInfoA* channelInfo,PortInfoA* mainPort,PortInfoA* slavPort);
COMMUNICATIONDLL_API ST_INT  CmnAddDevice(int channelId,DeviceInfoA* deviceInfo);
COMMUNICATIONDLL_API ST_VOID CmnStopChannel(int channelId);
COMMUNICATIONDLL_API ST_VOID CmnResumeChannel(int channelId);
#else
ST_VOID CmnInit();
ST_VOID CmnUnInit();
ST_VOID CmnStartup();
ST_VOID CmnStop();
ST_VOID CmnExcCmd(ProtocolTask task,ST_BOOLEAN ignorePort);
ST_VOID CmnRegistTaskBack(ProtocolTaskBack func);
ST_VOID CmnRegistMessageBack(ShowMessageBack func);
ST_VOID CmnRegistMessageBackEx(ShowMessageBackEx func,ST_VOID *callObj);
ST_VOID CmnRegistSourceView(int channelId,ST_BOOLEAN showView);

ST_INT  CmnAddChannel(ChannelInfoA channelInfo,PortInfoA* mainPort,PortInfoA* slavPort);
ST_INT  CmnAddDevice(int channelId,DeviceInfoA deviceInfo);
ST_VOID CmnStopChannel(int channelId);
ST_VOID CmnResumeChannel(int channelId);
#endif

#ifdef __cplusplus
}
#endif

#endif // CMNINTERFACE_H
