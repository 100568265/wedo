//#include "stdafx.h"
#include "CmnInterface.h"

Communication g_Cmn;

ST_VOID CmnInit()
{
    g_Cmn.Init();
}

ST_VOID CmnStartup()
{
    g_Cmn.Work();
}

ST_VOID CmnStop()
{
    g_Cmn.Stop();
}

ST_VOID  CmnUnInit()
{
    g_Cmn.Uninit();
}

ST_VOID CmnExcCmd(ProtocolTask task,ST_BOOLEAN ignorePort)
{
    g_Cmn.ExcCommand(task,ignorePort);
}

ST_VOID CmnRegistTaskBack(ProtocolTaskBack func)
{
    g_Cmn.RegistProtocolTaskBack(func);
}

ST_VOID CmnRegistMessageBackEx(ShowMessageBackEx func,ST_VOID *callObj)
{
	g_Cmn.RegistShowMessageBackEx(func,callObj);
}

ST_VOID CmnRegistMessageBack(ShowMessageBack func)
{
	g_Cmn.RegistShowMessageBack(func);
}


ST_VOID CmnRegistSourceView(int channelId,ST_BOOLEAN showView)
{
	g_Cmn.RegistSourceView(channelId,showView);
}

ST_INT  CmnAddChannel(ChannelInfoA* channelInfo,PortInfoA *mainPort,PortInfoA *slavPort)
{
	return g_Cmn.AddChannel(channelInfo,mainPort,slavPort);
}
ST_INT  CmnAddDevice(int channelId,DeviceInfoA* deviceInfo)
{
	return g_Cmn.AddDevice(channelId,deviceInfo);
}
