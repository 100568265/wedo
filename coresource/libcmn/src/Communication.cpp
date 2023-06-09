//#include "stdafx.h"
#include "Communication.h"
#include "Channel.h"
#include "syslogger.h"
#include "Dispatcher.h"
#include "MonitorServer.h"
#include "ActualValueServer.h"

#include <time.h>
#include <signal.h>
#include <assert.h>

#include "ATSHAValidator.h"

class AuthHelper
{
	AuthHelper(Communication * obj = NULL):
		_obj(obj), _interval(0), _count(0)
	{
		srand (time(0));
		_Thread.Start (ValidHandler, this, true);
	}
public:
	~AuthHelper() {
		_Thread.Stop();
	}

	void ErrorHandler () {
		if (_count++ < 10)
			return;

        SysLogger::GetInstance()->LogWarn ("auth error.");
		if (_obj)
			_obj->Stop();
		else
			abort();
		_count = 0;
	}

	static AuthHelper & Instance (Communication * obj)
	{
		static AuthHelper a(obj);
		return a;
	}

	static void * ValidHandler (void * param) {

        sleep (1);
        if (((AuthHelper*)param)->_interval --> 0)
        	return 0;

		ATSHAValidator atsha;
		if (atsha.Auth() != "Success")
			((AuthHelper*)param)->ErrorHandler();
        else {
            time_t now = time(0);
            SysLogger::GetInstance()->LogWarn ("%s auth success.", ctime(&now));

            ((AuthHelper*)param)->_count = 0;
        }

		((AuthHelper*)param)->_interval = (rand() % 1800) + 180;

		return 0;
	}
private:
	Thread          _Thread;
	Communication * _obj;
	int 			_interval;
	int 			_count;
};

Communication::Communication():
m_Inited(false),
m_Working(false),
m_pLogger(SysLogger::GetInstance()),
m_Channels(NULL),
m_ChannelInfos(NULL),
m_pDEngine(NULL),
m_ProtocolTaskBack(NULL),
m_ShowMessageBackEx(NULL),
m_ShowMessageBack(NULL),
m_pCallObj(NULL),
m_pMonitor(NULL),
m_pActual(NULL)
{
}

Communication::~Communication()
{
    if(NULL != m_ChannelInfos) {
        m_ChannelInfos->Clear();
        delete m_ChannelInfos;
        m_ChannelInfos = NULL;
    }
	if(m_Inited)
		Uninit();
}

static inline ST_VOID ingore_signal()
{
#ifdef _WIN32
	signal (SIGPIPE, SIG_IGN);
#else
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags   = 0;
    sigaction(SIGPIPE, &sa, 0 );
#endif
}

ST_VOID Communication::Init()
{
    if(m_Inited) return;
    ingore_signal();
    if(NULL == m_Channels) {
        m_Channels = CreateChannels();
    }
    for(int i = 0; i < m_Channels->GetCount(); i++) {
        Channel *m_pChannel = m_Channels->GetItem(i);
        m_pChannel->Init();
    }
	 if(NULL == m_pDEngine) {
        m_pDEngine = CreateEngine();
    }
    assert(m_pDEngine != NULL);
    m_pDEngine->Init();

	m_pMonitor = new MonitorServer();
	m_pMonitor->Init();

    m_pActual = new ActualValueServer();
    m_pActual->Init();
    //m_pLogger->GetInstance()->LogDebug("m_init = %d",m_Inited);

    m_Inited = true;
}

ST_VOID Communication::Init(ST_INT channelId, ProtocolBase *protocol)
{
    if(m_Inited) return;
    ingore_signal();
    if(NULL == m_Channels) {
        m_Channels = CreateChannels();
    }
    for(int i = 0; i < m_Channels->GetCount(); i++) {
        Channel *m_pChannel = m_Channels->GetItem(i);
        m_pChannel->Init();
    }
	 if(NULL == m_pDEngine) {
        m_pDEngine = CreateEngine();
    }
    assert(m_pDEngine != NULL);
    m_pDEngine->Init();
	m_pMonitor = new MonitorServer();
	m_pMonitor->Init();
	m_pActual = new ActualValueServer();
	m_pActual->Init();
    m_Inited = true;
}

ST_VOID Communication::Uninit()
 {
    if(!m_Inited)  return;
    for(int i = 0; i < m_Channels->GetCount(); i++) {
        Channel *m_pChannel = m_Channels->GetItem(i);
        assert(m_pChannel != NULL);
		m_pChannel->Stop();
        m_pChannel->Uninit();
    }
    assert(m_pDEngine!=NULL);
    m_pDEngine->Uninit();

	if(NULL != m_Channels) {
        m_Channels->Clear();
        delete m_Channels;
        m_Channels = NULL;
    }
    if(NULL != m_pDEngine) {
        delete m_pDEngine;
        m_pDEngine = NULL;
    }
	if(NULL != m_pMonitor) {
		delete m_pMonitor;
		m_pMonitor = NULL;
	}

    if(NULL != m_pActual){
        delete m_pActual;
        m_pActual = NULL;
    }

    m_Inited=false;
 }

ST_VOID Communication::Work()
{
	if(m_Working) return;
    for(int i = 0;i < m_Channels->GetCount();i++){
        Channel *m_pChannel=m_Channels->GetItem(i);
        assert(m_pChannel!=NULL);
        m_pChannel->Work();
    }
	m_pMonitor->Work();
//	AuthHelper::Instance(this);
    m_pActual->Work();
	m_Working = true;
}

ST_VOID Communication::Stop()
{
	if(!m_Working) return;
    for(int i = 0;i < m_Channels->GetCount();i++)
    {
        Channel *m_pChannel=m_Channels->GetItem(i);
        assert(m_pChannel!=NULL);
        m_pChannel->Stop();
    }
	m_Working=false;
}

ST_BOOLEAN Communication::IsWorking()
{
    return m_Working;
}

Dispatcher *Communication::GetEngine()
{
    return m_pDEngine;
}

List<Channel> *Communication::GetChannels()
{
    return m_Channels;
}

Channel *Communication::GetChannel(ST_INT channelId)
{
	if (!m_Channels) return NULL;
    for(int i = 0; i < m_Channels->GetCount(); i++)
    {
        Channel *m_pChannel = m_Channels->GetItem(i);
        if(m_pChannel->GetLocalChannelID() == channelId)
            return m_pChannel;
    }
    return NULL;
}

 List<Channel> *Communication::CreateChannels()
 {
        ChannelConfig chConfig;
        Channel *m_pChannel=NULL;
		if(m_ChannelInfos==NULL)
		{
			m_ChannelInfos=chConfig.LoadChannelConfig();
		}
        m_Channels=new List<Channel>();
		if(m_ChannelInfos==NULL) return m_Channels;
        for(int i=0;i<m_ChannelInfos->GetCount();i++)
        {
            m_pChannel=new Channel(this,m_ChannelInfos->GetItem(i));
            m_Channels->Add(m_pChannel);
        }
        return m_Channels;
 }



 Dispatcher *Communication::CreateEngine()
 {
        Dispatcher *pEngine=new Dispatcher(this);
        return pEngine;
 }


ST_VOID Communication::ExcCommand(ProtocolTask &task,ST_BOOLEAN ignorePort)
{
	task.taskResult.resultCode = -1;
	task.taskTime = time(0);//Thread::GetCurTime();
	ExcCommand(task.channelId,task,ignorePort);
}

ST_VOID Communication::ExcCommand(ST_INT channelId,ProtocolTask &task,ST_BOOLEAN ignorePort)
{
	Channel *m_pChannel = GetChannel(task.channelId);
	if(NULL!=m_pChannel){
		m_pChannel->GetCEngine()->OnTask(task,ignorePort);
	}
}

ST_VOID Communication::OnTaskBack(const ProtocolTask &task)
{
    if(NULL!=m_ProtocolTaskBack){
        m_ProtocolTaskBack(task);
    }
}

ST_VOID Communication::RegistProtocolTaskBack(ProtocolTaskBack backFunc)
{
    m_ProtocolTaskBack=backFunc;
}

ST_VOID Communication::RegistShowMessageBackEx(ShowMessageBackEx showMsgFunc,ST_VOID *callObj)
{
    m_pCallObj=callObj;
    m_ShowMessageBackEx=showMsgFunc;
}

ST_VOID Communication::RegistShowMessageBack(ShowMessageBack showMsgFunc)
{
    m_ShowMessageBack=showMsgFunc;
}

ST_VOID Communication::RegistSourceView(ST_INT channelId,ST_BOOLEAN enable)
{
	Channel* m_pChannel=GetChannel(channelId);
	if(m_pChannel!=NULL){
		m_pChannel->RegistSourceView(enable);
	}
}

ST_VOID Communication::OnShowMessage(const ST_CHAR *msg, ST_INT channelId, ST_INT deviceId)
{
	try {
		if(m_ShowMessageBack != NULL) {
			m_ShowMessageBack(msg, channelId, deviceId);
		}
		if(m_ShowMessageBackEx != NULL) {
			m_ShowMessageBackEx(msg, channelId, deviceId, m_pCallObj);
		}
		m_pMonitor->SendMessage(msg,channelId, deviceId);
	}
	catch(...) {
		m_pLogger->LogWarn("Exception : In Comm OnShowMessage and msg[%s]", msg);
	}
}

ST_INT Communication::AddChannel(ChannelInfoA* channel,PortInfoA *mainPort,PortInfoA *slavPort)
{
	if(m_ChannelInfos == NULL)
	{
		m_ChannelInfos = new List<ChannelInfo>();
	}
	assert(channel != NULL);
	ChannelInfo *channelInfo=new ChannelInfo();
	channelInfo->AutoOpen=channel->AutoOpen;
	m_pLogger->LogDebug("channel->AutoOpen=%d",channel->AutoOpen);

	channelInfo->ChannelID=channel->ChannelID;
	m_pLogger->LogDebug("channel->ChannelID=%d",channel->ChannelID);

	channelInfo->ChannelInterval=channel->ChannelInterval;
	m_pLogger->LogDebug("channel->ChannelInterval=%d",channel->ChannelInterval);

	Strcpy(channelInfo->ChannelName,channel->ChannelName);

	m_pLogger->LogDebug("Channel Name = %s",channel->ChannelName);

	channelInfo->ProtocolType=channel->ProtocolType;

	Strcpy(channelInfo->ProtocolFile,channel->ProtocolFile);
	m_pLogger->LogDebug("channel->ProtocolFile=%s",channel->ProtocolFile);

	channelInfo->EnableTransfer = channel->EnableTransfer;

	Strcpy(channelInfo->CtlProtocolFile,channel->CtlProtocolFile);
	m_pLogger->LogDebug("channel->CtlProtocolFile=%s",channel->CtlProtocolFile);

	if(mainPort!=NULL)
	{
		channelInfo->MainPort=new PortInfo();
		channelInfo->MainPort->PortType=mainPort->PortType;
		m_pLogger->LogDebug("main Port PortType = %d",mainPort->PortType);
		Strcpy(channelInfo->MainPort->PortName,mainPort->PortName);
		m_pLogger->LogDebug("main Port PortName = %s",mainPort->PortName);
		channelInfo->MainPort->LocalPort=mainPort->LocalPort;
		m_pLogger->LogDebug("main Port LocalPort = %d",mainPort->LocalPort);
		channelInfo->MainPort->RemotePort=mainPort->RemotePort;
		m_pLogger->LogDebug("main Port RemotePort = %d",mainPort->RemotePort);
		Strcpy(channelInfo->MainPort->LocalAddress,mainPort->LocalAddress);
		m_pLogger->LogDebug("main Port LocalAddress = %s",mainPort->LocalAddress);
		Strcpy(channelInfo->MainPort->RemoteAddress,mainPort->RemoteAddress);
		m_pLogger->LogDebug("main Port RemoteAddress = %s",mainPort->RemoteAddress);
		channelInfo->MainPort->Multicast=mainPort->Multicast;
		m_pLogger->LogDebug("main Port Multicast = %d",mainPort->Multicast);
		channelInfo->MainPort->PortNum=mainPort->PortNum;
		m_pLogger->LogDebug("main Port PortNum = %d",mainPort->PortNum);
		channelInfo->MainPort->BaudRate=mainPort->BaudRate;
		m_pLogger->LogDebug("main Port BaudRate = %d",mainPort->BaudRate);
		channelInfo->MainPort->DataBits=mainPort->DataBits;
		m_pLogger->LogDebug("main Port DataBits = %d",mainPort->DataBits);
		channelInfo->MainPort->StopBits=mainPort->StopBits;
		m_pLogger->LogDebug("main Port StopBits = %d",mainPort->StopBits);
		channelInfo->MainPort->Parity=mainPort->Parity;
		channelInfo->MainPort->IsRemoteCtrl=mainPort->IsRemoteCtrl;

        //liuxiaobin ADD
        Strcpy(channelInfo->MainPort->client_id,mainPort->client_id);
		m_pLogger->LogDebug("main Port client_id = %s",mainPort->client_id);

		Strcpy(channelInfo->MainPort->pub_topic,mainPort->pub_topic);
		m_pLogger->LogDebug("main Port pub_topic = %s",mainPort->pub_topic);

		Strcpy(channelInfo->MainPort->user,mainPort->user);
		m_pLogger->LogDebug("main Port user = %s",mainPort->user);

		Strcpy(channelInfo->MainPort->passwd,mainPort->passwd);
		m_pLogger->LogDebug("main Port passwd = %s",mainPort->passwd);


		m_pLogger->LogDebug("create mainport success!");
	}
	else{
		m_pLogger->LogDebug("create mainport null!");
	}
	m_ChannelInfos->Add(channelInfo);
	return 0;
}

ST_INT Communication::AddDevice(ST_INT channelId,DeviceInfoA* deviceInfo)
{
	if(m_ChannelInfos == NULL)
	{
        m_pLogger->LogDebug("m_ChannelInfos == NULL");//linweiming
        return -1;
    }
	assert(deviceInfo != NULL);
	ChannelInfo* m_pChannelInfo = NULL;
	for(int i = 0; i<m_ChannelInfos->GetCount(); i++) {
		m_pChannelInfo = m_ChannelInfos->GetItem(i);
		if(m_pChannelInfo->ChannelID == channelId) {
            m_pLogger->LogDebug("m_pChannelInfo->ChannelID == channelId");//linweiming
			break;
		}
	}
	if(m_pChannelInfo == NULL)
	{
        m_pLogger->LogDebug("m_pChannelInfo == NULL");//linweiming
        return -2;
    }
	if(m_pChannelInfo->DeviceInfos == NULL){
        m_pLogger->LogDebug("m_pChannelInfo == NULL111111111");//linweiming
		m_pChannelInfo->DeviceInfos = new List<DeviceInfo>();
	}
	DeviceInfo *device = new DeviceInfo();
	Strcpy(device->DeviceName, deviceInfo->DeviceName);
	device->Address=deviceInfo->Address;
	Strcpy(device->ProtocolFile, deviceInfo->ProtocolFile);
	device->ReSend = deviceInfo->ReSend;
	device->Break = deviceInfo->Break;
	device->IsRun = deviceInfo->IsRun;
	device->DeviceId = deviceInfo->DeviceId;
	device->DataAreas = deviceInfo->DataAreas;
	device->DataAreasCount = deviceInfo->DataAreasCount;
	device->deviceaddr1 = deviceInfo->deviceaddr1;
	device->transDeviceId = deviceInfo->transDeviceId;
	device->Channel = deviceInfo->Channel;
	Strcpy(device->Deviceserialtype, deviceInfo->Deviceserialtype);
	Strcpy(device->DeviceTypeID, deviceInfo->DeviceTypeID);
	device->ParentArea = deviceInfo->ParentArea;
	Strcpy(device->ProtocolName, deviceInfo->ProtocolName);
	Strcpy(device->Addressex, deviceInfo->Addressex);
	m_pChannelInfo->DeviceInfos->Add(device);
	return 0;
}

ST_INT Communication::RegistProtocol(ST_INT channelId,ST_INT deviceId,ST_VOID *protocol)
{
	if(m_ChannelInfos==NULL) return -1;
	ChannelInfo  *channelInfo=NULL;
	for(int i=0;i<m_ChannelInfos->GetCount();i++){
		channelInfo=m_ChannelInfos->GetItem(i);
		if(channelInfo->ChannelID==channelId){
			break;
		}
	}
	if(channelInfo==NULL) return -2;
	if(channelInfo->DeviceInfos==NULL) return -3;
	DeviceInfo  *deviceInfo=NULL;
	for(int i=0;i<channelInfo->DeviceInfos->GetCount();i++){
		deviceInfo=channelInfo->DeviceInfos->GetItem(i);
		if(deviceInfo->DeviceId==deviceId){
			break;
		}
	}
	if(deviceInfo==NULL) return -4;
	deviceInfo->Protocol=protocol;
	return 0;
}

