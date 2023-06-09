
#include "CommManager.h"

#include "Communication.h"
#include "VariableStorage.h"
#include "ChannelConfig.h"
#include "Device.h"

#include <stdio.h>
#include <stdlib.h>

#include "RtValueTransfer.h"
#include "watchdoghelper.h"

#include <time.h>
#include <sys/resource.h> // func setrlimit
//#include <execinfo.h>     // func backtrace
#include <sstream>
//#include "sysinifile.h"
//#include "dbmysql.h"

//==========================================================

const static int * reserved_memory = new int[1024];

static void no_memory_quitter () { abort(); }
static void no_memory_handler () {
    delete reserved_memory;
    reserved_memory = NULL;
    set_new_handler (no_memory_quitter);

    time_t now = time(0);
    SysLogger::GetInstance()->LogError ("[out of memory] %s "
        "Emergency function has been called, "
        "the result of the next call will terminate the program. "
        "If you see this message, "
        "please provide feedback to the developers to investigate the cause.", ctime(&now));
}

//==========================================================

void segment_fault_dump (int signo)
{
    void *array[24];
    size_t size = 0;//backtrace (array, 24);
    char **strings = 0;//backtrace_symbols (array, size);
    time_t now = time(0);
    std::stringstream msgbuf;
    msgbuf << ctime(&now);
    msgbuf << " Obtained " << size <<  " stack frames.\r\n";

    for (size_t i = 0; i < size; i++)
        msgbuf << strings[i] << "\r\n";

//    free (strings);

    if (SIGSEGV == signo)
        msgbuf << "Segment Fault.\r\n";
    if (SIGBUS  == signo)
        msgbuf << "Bus Error.\r\n";
    SysLogger::GetInstance()->LogError(msgbuf.str().c_str());

//    struct rlimit nowrlimit = {RLIM_INFINITY, RLIM_INFINITY};
//    setrlimit(RLIMIT_CORE, &nowrlimit);
    abort();
}

//==========================================================

CCommManager::CCommManager ():
m_pComm(new Communication  ()),
m_pSave(new VariableStorage()),
m_pWdog(new WatchdogHelper ())
{
    signal (SIGSEGV, &segment_fault_dump); // Segment Fault signal capturing
    signal (SIGBUS , &segment_fault_dump);
    signal (SIGHUP , SIG_IGN);             // SSH disconnect signal capturing

    set_new_handler (no_memory_handler);

    m_pWdog->Init();

    RtValueTransfer::Instance().Init("35722");
}

CCommManager::~CCommManager()
{
    this->Stop();
//	if(m_pMySql != NULL){
//		delete m_pMySql;
//		m_pMySql = NULL;
//	}
    if (m_pSave)
        delete m_pSave;
}

/*
ST_INT CCommManager::ConnectDatabase()
{
	int ret = 0;
	CIniFile iniFile(".//Config//DBConfig.ini");
	ret = iniFile.OpenIni();
	if(!ret) return ret;
	serverName = iniFile.GetValue("DBConfig","ServerName");
	string port = iniFile.GetValue("DBConfig","ServerPort");
	serverPort = atoi(port.c_str());
	dbName = iniFile.GetValue("DBConfig","DatabaseName");
	userName = iniFile.GetValue("DBConfig","UserName");
	passWord = iniFile.GetValue("DBConfig","Password");
    iniFile.CloseIni();
	ret = m_pMySql->Connect(serverName, userName, passWord, dbName, serverPort, 0);
	return ret;
}
*/

ST_INT CCommManager::LoadConfigs ()
{
    const char * cfname1 = "/etc/comm/channel.xml"  ;
    const char * tfname1 = "/etc/comm/template.xml" ;
    const char * cfname2 = "./channelconfig/channel.xml"  ;
    const char * tfname2 = "./channelconfig/template.xml" ;

    const char * cfname0 = 0;
    const char * tfname0 = 0;

    if (access (cfname2, F_OK) == 0) cfname0 = cfname2;
    if (access (tfname2, F_OK) == 0) tfname0 = tfname2;

    if (access (cfname1, F_OK) == 0) cfname0 = cfname1;
    if (access (tfname1, F_OK) == 0) tfname0 = tfname1;

	tinyxml2::XMLDocument chlDoc;
	tinyxml2::XMLDocument tmpDoc;

	if (chlDoc.LoadFile(cfname0)) {
	    printf("[Error] Error when loading the channels from the disk,\n"
	    	   "\tMay be file does not exist.\n");
	    return 0xE01;
	}
	bool isload = tmpDoc.LoadFile(tfname0);
	if  (isload) {
	    printf("[Warning] Not Found DeviceTemplateGroup from the disk.\n");
	}

	tinyxml2::XMLHandle chlHandle (&chlDoc);
	tinyxml2::XMLHandle tmpHandle(&tmpDoc);
	tinyxml2::XMLNode * chlNode  =  chlHandle.FirstChildElement("ArrayOfChannel").ToNode();
	tinyxml2::XMLNode * tmpNode = NULL;

	if (!isload) {
		tmpNode = tmpHandle.FirstChildElement("ArrayOfDeviceTemplateType").ToNode();
	}

	return LoadChannels(chlNode, tmpNode);
}

ST_INT CCommManager::LoadChannels(tinyxml2::XMLHandle chlHandle, tinyxml2::XMLHandle tmpHandle)
{
	if (! chlHandle.ToNode()) {
	    printf("[Error] Not Found channel,"
	    	"\tPlease verify that you have a channel parameter.\n");
	    exit(0xE03);
	}

	for (tinyxml2::XMLNode* node = chlHandle.FirstChildElement("Channel").ToNode();
        node; node = node->NextSibling()) {
		LoadChannel (node);
		LoadDevices (node->FirstChildElement("Devices"), tmpHandle);
	}

	return 0;
}

/**
 * @Author    Wedo
 * @NotesTime 2016-06-20T04:09:33+0800
 */
ST_INT CCommManager::LoadChannel (tinyxml2::XMLHandle handle)
{
    if (! handle.ToNode()) {
    	printf("[Warning] Not Found Channel element.\n");
        return -1;
    }
    ChannelInfoA *channelInfo = new ChannelInfoA();
    memset (channelInfo, 0, sizeof (ChannelInfoA));

    tinyxml2::XMLNode *
    node = handle.FirstChildElement("channelId").FirstChild().ToNode();
    if (node) channelInfo->ChannelID = atoi(node->Value());

    node = handle.FirstChildElement("protocolFile").FirstChild().ToNode();
    if (node) strncpy(channelInfo->ProtocolFile, node->Value(), 254);

    node = handle.FirstChildElement("protocolType").FirstChild().ToNode();
    if (node) channelInfo->ProtocolType = atoi(node->Value());

    node = handle.FirstChildElement("channelInterval").FirstChild().ToNode();
    if (node) channelInfo->ChannelInterval = atoi(node->Value());

    node = handle.FirstChildElement("channelName").FirstChild().ToNode();
    if (node) strncpy(channelInfo->ChannelName, node->Value(), 64);

    node = handle.FirstChildElement("autoOpen").FirstChild().ToNode();
    if (node) channelInfo->AutoOpen = atoi(node->Value());

    node = handle.FirstChildElement("transChannelId").FirstChild().ToNode();
    if (node) channelInfo->TransChannelID = atoi(node->Value());

    node = handle.FirstChildElement("IstransChannel").FirstChild().ToNode();
    if (node) channelInfo->EnableTransfer = atoi(node->Value());

//    node = handle.FirstChildElement("protocolName").FirstChild().ToNode();
//    if (!node) break;
    PortInfoA* mainPortInfo = LoadPort (handle.FirstChildElement("MainPort"));
    PortInfoA* backPortInfo = LoadPort (handle.FirstChildElement("BackPort"));

    if (!mainPortInfo && !backPortInfo)
        printf ("[Warning] No channel port type configured.\n");

    m_pComm->AddChannel(channelInfo, mainPortInfo, backPortInfo);

    if (channelInfo)  delete channelInfo;
    if (mainPortInfo) delete mainPortInfo;
    if (backPortInfo) delete backPortInfo;

    // LoadDevices (handle.FirstChildElement("Devices"));
    return 0;
}

/**
 * @Author    Wedo
 * @NotesTime 2016-05-20T04:09:28+0800
 */
PortInfoA* CCommManager::LoadPort (tinyxml2::XMLHandle handle)
{
    if (! handle.ToNode()) {
        return NULL;
    }
    PortInfoA* portInfo = new PortInfoA();
    memset (portInfo, 0, sizeof (PortInfoA));

    tinyxml2::XMLNode*
    node = handle.FirstChildElement("PortName").FirstChild().ToNode();
    if (node) strncpy(portInfo->PortName, node->Value(), 64);

    node = handle.FirstChildElement("PortType").FirstChild().ToNode();
    if (node) portInfo->PortType = atoi (node->Value());

    node = handle.FirstChildElement("PortNum").FirstChild().ToNode();
    if (node) portInfo->PortNum = atoi (node->Value());

    node = handle.FirstChildElement("BaudRate").FirstChild().ToNode();
    if (node) portInfo->BaudRate = atoi (node->Value());

    node = handle.FirstChildElement("Parity").FirstChild().ToNode();
    if (node) portInfo->Parity = atoi (node->Value());

    node = handle.FirstChildElement("DataBits").FirstChild().ToNode();
    if (node) portInfo->DataBits = atoi (node->Value());

    node = handle.FirstChildElement("StopBits").FirstChild().ToNode();
    if (node) portInfo->StopBits = atoi (node->Value());

    node = handle.FirstChildElement("LocalPort").FirstChild().ToNode();
    if (node) portInfo->LocalPort = atoi (node->Value());

    node = handle.FirstChildElement("RemotePort").FirstChild().ToNode();
    if (node) portInfo->RemotePort = atoi (node->Value());

    node = handle.FirstChildElement("IsMulticast").FirstChild().ToNode();
    if (node) portInfo->Multicast = atoi (node->Value());

    node = handle.FirstChildElement("LocalAddress").FirstChild().ToNode();
    if (node) strncpy(portInfo->LocalAddress, node->Value(), 64);

    node = handle.FirstChildElement("RemoteAddress").FirstChild().ToNode();
    if (node) strncpy(portInfo->RemoteAddress, node->Value(), 64);

    node = handle.FirstChildElement("isremotecontrol").FirstChild().ToNode();
    if (node) portInfo->IsRemoteCtrl = atoi (node->Value());

    node = handle.FirstChildElement("client_id").FirstChild().ToNode();
    if (node) strncpy(portInfo->client_id, node->Value(), 64);

    node = handle.FirstChildElement("pub_topic").FirstChild().ToNode();
    if (node) strncpy(portInfo->pub_topic, node->Value(), 64);

    node = handle.FirstChildElement("user").FirstChild().ToNode();
    if (node) strncpy(portInfo->user, node->Value(), 64);

    node = handle.FirstChildElement("passwd").FirstChild().ToNode();
    if (node) strncpy(portInfo->passwd, node->Value(), 64);

    //node = handle.FirstChildElement("client_id").FirstChild().ToNode();

    /*
    <client_id>123</client_id>
      <pub_topic>mailian/sigalarm</pub_topic>
      <user>zywl</user>
      <passwd>zywl123</passwd>
    */
//    node = handle.FirstChildElement("IsBackup").FirstChild().ToNode();
//    if (!node) break;

//    node = handle.FirstChildElement("PortId").FirstChild().ToNode();
//        if (!node) break;

//    node = handle.FirstChildElement("ChannelId").FirstChild().ToNode();
//        if (!node) break;

    return portInfo;
}

ST_INT CCommManager::LoadDevices (tinyxml2::XMLHandle chlHandle, tinyxml2::XMLHandle tmpHandle)
{
    if (! chlHandle.ToNode()) {
        printf("[Error] Not Found Devices.\n");
        return -1;
    }
    DeviceInfoA devinfo;
	for (tinyxml2::XMLNode* node = chlHandle.FirstChildElement("Device").ToNode();
        node; node = node->NextSibling()) {
        memset (&devinfo, 0, sizeof (DeviceInfoA)); // ex to null
        devinfo.ParamEx = NULL;
        LoadDevice(node, tmpHandle, devinfo);
        m_pComm->AddDevice(devinfo.Channel, &devinfo);
    }
    return 0;
}

ST_INT CCommManager::LoadDevice
	(tinyxml2::XMLHandle devHandle, tinyxml2::XMLHandle tmpHandle, DeviceInfoA& deviceInfo)
{
    if (! devHandle.ToNode()) {
        printf("[Error] Not Found Device element.\n");
        return -1;
    }
    tinyxml2::XMLNode* node = devHandle.FirstChildElement("deviceId").FirstChild().ToNode();
    if (node) deviceInfo.DeviceId = atoi (node->Value());

    node = devHandle.FirstChildElement("Addressex").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.Addressex, node->Value(), 254);

    node = devHandle.FirstChildElement("ParentArea").FirstChild().ToNode();
    if (node) deviceInfo.ParentArea = atoi (node->Value());

    node = devHandle.FirstChildElement("Address").FirstChild().ToNode();
    if (node) deviceInfo.Address = atoi (node->Value());

    node = devHandle.FirstChildElement("Address1").FirstChild().ToNode();
    if (node) deviceInfo.deviceaddr1 = atoi (node->Value());

    node = devHandle.FirstChildElement("Channel").FirstChild().ToNode();
    if (node) deviceInfo.Channel = atoi (node->Value());

    node = devHandle.FirstChildElement("Name").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.DeviceName, node->Value(), 64);

    node = devHandle.FirstChildElement("DeviceTypeID").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.DeviceTypeID, node->Value(), 254);

    node = devHandle.FirstChildElement("Deviceserialtype").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.Deviceserialtype, node->Value(), 254);

    node = devHandle.FirstChildElement("Run").FirstChild().ToNode();
    if (node) deviceInfo.IsRun = atoi (node->Value());

    node = devHandle.FirstChildElement("BreakCount").FirstChild().ToNode();
    if (node) deviceInfo.Break = atoi (node->Value());

    node = devHandle.FirstChildElement("Protocolfile").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.ProtocolFile, node->Value(), 255);

    node = devHandle.FirstChildElement("ResendCount").FirstChild().ToNode();
    if (node) deviceInfo.ReSend = atoi (node->Value());

    node = devHandle.FirstChildElement("ProtocolName").FirstChild().ToNode();
    if (node) strncpy(deviceInfo.ProtocolName, node->Value(), 254);

    node = devHandle.FirstChildElement("transDeviceId").FirstChild().ToNode();
    if (node) deviceInfo.transDeviceId = atoi (node->Value());

//    node = devHandle.FirstChildElement("Isbreak").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("DeviceTypeName").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("Attribute").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("OldName").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("OldNo").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("deviceTempTypeId").FirstChild().ToNode();

//    node = devHandle.FirstChildElement("deviceTempId").FirstChild().ToNode();


    // hhc add
    // handler device ex param
    node = devHandle.FirstChildElement("ParamExType").FirstChild().ToNode();
    if (node) deviceInfo.ParamExType = atoi(node->Value());


    switch(deviceInfo.ParamExType)
    {
    case 1: // DL645-07 deviceinfoex
        {
            node = devHandle.FirstChildElement("ParamEx").ToNode();
            if (node)
            {
                tinyxml2::XMLHandle exhandler(node);
                DeviceInfoEx_DL64507* ex = new DeviceInfoEx_DL64507();
                node = exhandler.FirstChildElement("Ctrl").FirstChild().ToNode();
                if (node) strncpy(ex->Ctrl, node->Value(), 8);

                node = exhandler.FirstChildElement("Date").FirstChild().ToNode();
                if (node) strncpy(ex->Date, node->Value(), 12);

                node = exhandler.FirstChildElement("Passwd").FirstChild().ToNode();
                if (node) strncpy(ex->Passwd, node->Value(), 6);
                deviceInfo.ParamEx = ex;
            }

        } break;
    default:
        deviceInfo.ParamEx = NULL;
        break;
    }


    // 加载变量
    for (tinyxml2::XMLNode* tmpNode = devHandle.FirstChildElement("GenerallyVariables"). // !has point
        FirstChildElement("GenerallyVariable").ToNode(); tmpNode; tmpNode = tmpNode->NextSibling())
        LoadGenerallyVariable (tmpNode);

    node = devHandle.FirstChildElement("deviceTempSerialId").FirstChild().ToNode();
    if (!node) return -1;
    /// 加载该设备拥有的设备模板
    tinyxml2::XMLNode* retnode = FindDeviceTemplateSerial (devHandle, tmpHandle);
    if (!retnode)
        return -1;
    // hhc change old
    // deviceInfo.DataAreas
    // = LoadDataAreas(tinyxml2::XMLHandle(retnode).FirstChildElement("DataAreas"), deviceInfo.DataAreasCount);


    // new for add Device Template ex param type
    deviceInfo.SerialType = SerialType::DEFAULT;
    node = tinyxml2::XMLHandle(retnode).FirstChildElement("devTempSerialType").FirstChild().ToNode();
    if (node)
    {
        if (!strcmp(node->Value(), "OPCUA数据配置控件"))
            deviceInfo.SerialType = SerialType::OPCUA;
        else if (!strcmp(node->Value(), "DL645_07数据配置控件"))
            deviceInfo.SerialType = SerialType::DL64507;
        else if (!strcmp(node->Value(), "DL645_97数据配置控件"))
            deviceInfo.SerialType = SerialType::DL64597;
    }


    // change
    deviceInfo.DataAreas
    = LoadDataAreas(tinyxml2::XMLHandle(retnode).FirstChildElement("DataAreas"), deviceInfo.DataAreasCount, deviceInfo.SerialType);
/*
    deviceInfo.ExDataAreas
    = LoadExDataAreas(tinyxml2::XMLHandle(retnode).FirstChildElement("DataAreas"), deviceInfo.DataAreasCount, deviceInfo.SerialType);*/

    deviceInfo.ExDataAreas = new ST_DeviceDataAreaEx[deviceInfo.DataAreasCount];
    for(int i=0;i<deviceInfo.DataAreasCount;i++){
        deviceInfo.ExDataAreas[i].items = new ST_DataAreaItemEx[deviceInfo.DataAreas[i].itemCount];
    }

    LoadExDataAreas(tinyxml2::XMLHandle(retnode).FirstChildElement("DataAreas"),deviceInfo.ExDataAreas,deviceInfo.DataAreasCount,deviceInfo.SerialType);
    return 0;
}


tinyxml2::XMLNode* CCommManager::FindDeviceTemplateSerial (tinyxml2::XMLHandle devHandle, tinyxml2::XMLHandle tmpHandle)
{
    if (! devHandle.ToNode()) {
        printf("[Error] Not Found Device element.\n");
        return NULL;
    }
    if (! tmpHandle.ToNode()) {
        printf("[Error] Not Found DeviceTemplateSerial element.\n");
        return NULL;
    }

    tinyxml2::XMLNode *
    compareNode = devHandle.FirstChildElement("deviceTempTypeId").FirstChild().ToNode();
    std::string
    comparestr = (compareNode ? compareNode->Value() : "");
    tinyxml2::XMLNode* devtemptypeNode = tmpHandle.FirstChildElement("DeviceTemplateType").ToNode();
    for (; devtemptypeNode; devtemptypeNode = devtemptypeNode->NextSibling())
    {
        tinyxml2::XMLNode* node = tinyxml2::XMLHandle(devtemptypeNode).FirstChildElement("devTempTypeId").FirstChild().ToNode();
        if (node && comparestr == node->Value())
            break;
    }
    if (!devtemptypeNode)
        return NULL;

    compareNode = devHandle.FirstChildElement("deviceTempId").FirstChild().ToNode();
    comparestr = (compareNode ? compareNode->Value() : "");
    tinyxml2::XMLNode* devtempNode = tinyxml2::XMLHandle(devtemptypeNode).
                    FirstChildElement("DeviceTemplates").FirstChildElement("DeviceTemplate").ToNode();
    for (; devtempNode; devtempNode = devtempNode->NextSibling())
    {
        tinyxml2::XMLNode* node = tinyxml2::XMLHandle(devtempNode).FirstChildElement("devTempId").FirstChild().ToNode();
        if (node && comparestr == node->Value())
            break;
    }
    if (!devtempNode)
        return NULL;

    compareNode = devHandle.FirstChildElement("deviceTempSerialId").FirstChild().ToNode();
    comparestr = (compareNode ? compareNode->Value() : "");
    tinyxml2::XMLNode* devtempsrlNode = tinyxml2::XMLHandle(devtempNode).
                        FirstChildElement("DeviceTemplateSerials").FirstChildElement("DeviceTemplateSerial").ToNode();
    for (; devtempsrlNode; devtempsrlNode = devtempsrlNode->NextSibling())
    {
        tinyxml2::XMLNode* node = tinyxml2::XMLHandle(devtempsrlNode).FirstChildElement("devTempSerialId").FirstChild().ToNode();
        if (node && comparestr == node->Value())
            return devtempsrlNode;
    }
    return NULL;
}

// hhc change new add third param
DeviceDataArea* CCommManager::LoadDataAreas (tinyxml2::XMLHandle handle, int& areasCount, int& serialType)
{
    if (! handle.ToNode()) {
        printf("[Error] Not Found DataAreas element.\n");
        return NULL;
    }
    /*if (__size <= 0) {
        return NULL;
    }
    DeviceDataArea* dataareas = new DeviceDataArea[__size];

    int it = 0;
    for (tinyxml2::XMLNode* node = handle.FirstChildElement("DataArea").ToNode();
        node && it < __size; node = node->NextSibling(), ++it) {
        LoadDataArea (node, dataareas[it]);
    }*/

    std::map<long, tinyxml2::XMLNode*> sortctr;
    tinyxml2::XMLNode* node = handle.FirstChildElement("DataArea").ToNode();
    for (; node; node = node->NextSibling()) {
    	tinyxml2::XMLNode* tempnode = tinyxml2::XMLHandle(node).FirstChildElement("Sort").FirstChild().ToNode();
    	if (!tempnode) continue; long tempno = atoi(tempnode->Value());
    	sortctr.insert (std::pair<long, tinyxml2::XMLNode*>(tempno, node)); // sort and dataarea node
    }
    if (sortctr.empty())
    	return NULL;
    areasCount  = sortctr.size();
    ST_DeviceDataArea* dataareas = new ST_DeviceDataArea[areasCount];

    int index = 0;
    for (std::map<long, tinyxml2::XMLNode*>::iterator it = sortctr.begin();
    	it != sortctr.end() && index <areasCount  ; ++it, ++index)
    	LoadDataArea (it->second, dataareas[index], serialType); // hhc change

    return dataareas;
}

// hha change add the third param
ST_INT CCommManager::LoadDataArea (tinyxml2::XMLHandle handle, DeviceDataArea& dataarea, int& serialType)
{
    if (! handle.ToNode()) {
        printf("[Error] Not Found DataArea element.\n");
        return -1;
    }
    memset (&dataarea, 0, sizeof(DeviceDataArea));

    tinyxml2::XMLNode* node = handle.FirstChildElement("DataUnitLen").FirstChild().ToNode();
    if (node) dataarea.dataUnitLen = atoi(node->Value());

//    node = handle.FirstChildElement("devTempSerialId").ToNode();

//    node = handle.FirstChildElement("AreaDataAddrOffset").ToNode();

    node = handle.FirstChildElement("DataLen").FirstChild().ToNode();
    if (node) dataarea.len = atoi(node->Value());

    node = handle.FirstChildElement("AreaAddr").FirstChild().ToNode();
    if (node) dataarea.addr = atoi(node->Value());

    node = handle.FirstChildElement("dataAreaName").FirstChild().ToNode();
    if (node) strncpy(dataarea.areaName, node->Value(), 64);

    node = handle.FirstChildElement("ReadCode").FirstChild().ToNode();
    if (node) dataarea.readCode = atoi(node->Value());

    node = handle.FirstChildElement("WriteCode").FirstChild().ToNode();
    if (node) dataarea.writeCode = atoi(node->Value());

//    node = handle.FirstChildElement("dataAreaId").FirstChild().ToNode();

//    node = handle.FirstChildElement("Sort").FirstChild().ToNode();

//    node = handle.FirstChildElement("Items").FirstChild().ToNode();
/*
    node = handle.FirstChildElement("DataItemCount").FirstChild().ToNode();
    if (node) dataarea.itemCount = atoi(node->Value());

    node = handle.FirstChildElement("ParamExType").FirstChild().ToNode();
    if (node) exdataare.paramExType = atoi(node->Value());

    // dataarea ex param load
    switch(serialType)
    {
    default:
        exdataare.paramEx = NULL;
        break;
    }*/


    dataarea.items = LoadDataItems(handle.FirstChildElement("DataItems"), dataarea.itemCount, serialType); // hhc change

    return 0;
}



/**
 * [CCommManager::LoadDataItems  Load all DataItem]
 * @Author    Wedo
 * @NotesTime 2016-07-09T04:09:08+0800
 */
 // hhc change new add third param
ST_DataAreaItem* CCommManager::LoadDataItems (tinyxml2::XMLHandle handle, int& _sizeret, int& serialType)
{
    if (! handle.ToNode()) {
        printf("[Error] Not Found DataItems element.\n");
        return NULL;
    }

    std::map<long, tinyxml2::XMLNode*> sortctr;
    tinyxml2::XMLNode* node = handle.FirstChildElement("DataItem").ToNode();
    for (; node; node = node->NextSibling()) {
    	tinyxml2::XMLNode* tempnode = tinyxml2::XMLHandle(node).FirstChildElement("Sort").FirstChild().ToNode();
    	if (!tempnode) continue; long tempno = atoi(tempnode->Value());
    	sortctr.insert (std::pair<long, tinyxml2::XMLNode*>(tempno, node));
    }
    if (sortctr.empty())
    	return NULL;
    _sizeret = sortctr.size();
    ST_DataAreaItem* dataitems = new ST_DataAreaItem[_sizeret];


    int index = 0;
    for (std::map<long, tinyxml2::XMLNode*>::iterator it = sortctr.begin();
    	it != sortctr.end() && index < _sizeret; ++it, ++index)
    	LoadDataItem (it->second, &dataitems[index], serialType);
    return dataitems;
}


// hhc new add
inline ST_BYTE AscToHex(ST_BYTE aChar)
{
    if((aChar>=0x30)&&(aChar<=0x39))
        aChar -= 0x30;
    else if((aChar>=0x41)&&(aChar<=0x46))//大写字母
        aChar -= 0x37;
    else if((aChar>=0x61)&&(aChar<=0x66))//小写字母
        aChar -= 0x57;
    else
        aChar = 0xff;
    return aChar;
}

/**
 * [CCommManager::LoadDataItem  Load all properties from current data item.]
 * @Author    Wedo
 * @NotesTime 2016-07-09T04:02:59+0800
 */
 // dataitem reference change to pointer,and add the third param
ST_INT CCommManager::LoadDataItem (tinyxml2::XMLHandle handle, ST_DataAreaItem* dataitem, int& serialType)
{
	if (! handle.ToNode()) {
        printf("[Error] Not Found DataItem element.\n");
        return -1;
    }


    tinyxml2::XMLNode* node = NULL;

    // hhc change
    // load ex param
    /*
    node = handle.FirstChildElement("ParamExType").FirstChild().ToNode();
    if (node) dataitem->paramExType = atoi(node->Value());

    switch(serialType)
    {
    case SerialType::OPCUA:
        {
            DataAreaItemEx_OPCUA* ex = new DataAreaItemEx_OPCUA();
            node = handle.FirstChildElement("ParamEx").ToNode();
            if (node)
            {
                tinyxml2::XMLHandle itemex(node);
                node = itemex.FirstChildElement("NodeId").FirstChild().ToNode();
                if (node)
                    ex->nodeid = node->Value();
                dataitem->paramEx = ex;
            }
        }break;
    case SerialType::DL64507:
        {
            DataAreaItemEx_64507* ex = new DataAreaItemEx_64507();
            node = handle.FirstChildElement("ParamEx").ToNode();
            if (node)
            {
                tinyxml2::XMLHandle itemex(node);
                node = itemex.FirstChildElement("ParseType").FirstChild().ToNode();
                if (node) ex->parsetype = atoi(node->Value());


                node = itemex.FirstChildElement("DataId").FirstChild().ToNode();
                if (node)
                {
                    // DI3 - DI0
                    char buf[20];
                    strncpy(buf, node->Value(), 20);
                    ex->dataid = AscToHex(buf[0])*16 + AscToHex(buf[1]);
                    ex->dataid = ex->dataid << 8;
                    ex->dataid += AscToHex(buf[2])*16 + AscToHex(buf[3]);
                    ex->dataid = ex->dataid << 8;
                    ex->dataid += AscToHex(buf[4])*16 + AscToHex(buf[5]);
                    ex->dataid = ex->dataid << 8;
                    ex->dataid += AscToHex(buf[6])*16 + AscToHex(buf[7]);
                }

                node = itemex.FirstChildElement("KMState").FirstChild().ToNode();
                if (node) ex->kmstate = atoi(node->Value());


                dataitem->paramEx = ex;
            }
        }break;
    case SerialType::DL64597:
        {
            dataitem->paramEx = NULL;

        }break;
    default:
        break;
    }*/

    node = handle.FirstChildElement("Id").FirstChild().ToNode();
    if (node) dataitem->id = atoi(node->Value());

//    node = handle.FirstChildElement("dataItemId").ToNode();

//    node = handle.FirstChildElement("dataAreaId").ToNode();

    node = handle.FirstChildElement("dataItemName").FirstChild().ToNode();
    if (node) strncpy(dataitem->itemName, node->Value(), 64);

    node = handle.FirstChildElement("DataType").FirstChild().ToNode();
    if (node) dataitem->dataType = atoi(node->Value());

    node = handle.FirstChildElement("Datalen").FirstChild().ToNode();
    if (node) dataitem->dataLen = atoi(node->Value());

    node = handle.FirstChildElement("BeginBit").FirstChild().ToNode();
    if (node) dataitem->beginBit = atoi(node->Value());

    node = handle.FirstChildElement("EndBit").FirstChild().ToNode();
    if (node) dataitem->endBit = atoi(node->Value());

    node = handle.FirstChildElement("Addr").FirstChild().ToNode();
    if (node) dataitem->addr = atoi(node->Value());

    node = handle.FirstChildElement("Coeficient").FirstChild().ToNode();
    if (node) {
        dataitem->coeficient = atof(node->Value());
        if (dataitem->coeficient == 0.0)
            dataitem->coeficient =  1.0;
    }

//    node = handle.FirstChildElement("Order").ToNode();

    node = handle.FirstChildElement("CodeType").FirstChild().ToNode();
    if (node) dataitem->codeType = atoi(node->Value());


    // hhc new add



//    node = handle.FirstChildElement("DestDataType").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestAttribute").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestAppType").FirstChild().ToNode();

//   node = handle.FirstChildElement("DestMinValiedValue").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestMaxValiedValue").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestRestoreDistance").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestCofficient").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestZero").FirstChild().ToNode();

//    node = handle.FirstChildElement("DestOffset").FirstChild().ToNode();
//    if (node) dataitem->destOffset = atoi(node->Value());

//    node = handle.FirstChildElement("Decimal").FirstChild().ToNode();

//    node = handle.FirstChildElement("EventType").FirstChild().ToNode();

//    node = handle.FirstChildElement("Overlimit").FirstChild().ToNode();

//    node = handle.FirstChildElement("Sort").FirstChild().ToNode();
    return 0;
}

ST_INT CCommManager::LoadExDataAreas (tinyxml2::XMLHandle handle,ST_DeviceDataAreaEx *dataeares, int areasCount,int serialType)
{
    if (! handle.ToNode()) {
        printf("[Error] Not Found DataAreas element.\n");
        return NULL;
    }

    int index = 0 ;
    tinyxml2::XMLNode* node = handle.FirstChildElement("DataArea").ToNode();
    for (; node; node = node->NextSibling(),index++ ) {

    	tinyxml2::XMLNode* tempnode = tinyxml2::XMLHandle(node).FirstChildElement("ParamExType").FirstChild().ToNode();
        if (tempnode) dataeares[index].paramExType = atoi(node->Value());

        tinyxml2::XMLHandle itemsHandle = tinyxml2::XMLHandle(node).FirstChildElement("DataItems");

        ST_DataAreaItemEx *dataitems = dataeares[index].items;

        int itemIndex = 0;
        tinyxml2::XMLNode* itemNode = itemsHandle.FirstChildElement("DataItem").ToNode();
        for (; itemNode; itemNode = itemNode->NextSibling(),itemIndex++) {
            tinyxml2::XMLHandle itemhandle =  itemNode;

            tinyxml2::XMLNode* valuenode = itemhandle.FirstChildElement("ParamExType").FirstChild().ToNode();
            if (valuenode) dataitems[itemIndex].paramExType = atoi(valuenode->Value());

            valuenode = itemhandle.FirstChildElement("DestOffset").FirstChild().ToNode();
            if (valuenode) dataitems[itemIndex].destOffset = atoi(valuenode->Value());
//            printf("DestOffset :%d\n",atoi(valuenode->Value()));

            switch(serialType)
            {
            case SerialType::OPCUA:
                {
                    DataAreaItemEx_OPCUA* ex = new DataAreaItemEx_OPCUA();
                    valuenode = itemhandle.FirstChildElement("ParamEx").ToNode();
                    if (valuenode)
                    {
                        tinyxml2::XMLHandle itemex(valuenode);
                        valuenode = itemex.FirstChildElement("NodeId").FirstChild().ToNode();
                        if (valuenode)
                            ex->nodeid = valuenode->Value();
                        dataitems[itemIndex].paramEx = ex;
                    }
                }break;
            case SerialType::DL64507:
                {
                    DataAreaItemEx_64507* ex = new DataAreaItemEx_64507();
                    valuenode = itemhandle.FirstChildElement("ParamEx").ToNode();
                    if (valuenode)
                    {
                        tinyxml2::XMLHandle itemex(valuenode);
                        valuenode = itemex.FirstChildElement("ParseType").FirstChild().ToNode();
                        if (valuenode) ex->parsetype = atoi(valuenode->Value());


                        valuenode = itemex.FirstChildElement("DataId").FirstChild().ToNode();
                        if (valuenode)
                        {
                            // DI3 - DI0
                            char buf[20];
                            strncpy(buf, valuenode->Value(), 20);
                            ex->dataid = AscToHex(buf[0])*16 + AscToHex(buf[1]);
                            ex->dataid = ex->dataid << 8;
                            ex->dataid += AscToHex(buf[2])*16 + AscToHex(buf[3]);
                            ex->dataid = ex->dataid << 8;
                            ex->dataid += AscToHex(buf[4])*16 + AscToHex(buf[5]);
                            ex->dataid = ex->dataid << 8;
                            ex->dataid += AscToHex(buf[6])*16 + AscToHex(buf[7]);
                        }

                        valuenode = itemex.FirstChildElement("KMState").FirstChild().ToNode();
                        if (valuenode) ex->kmstate = atoi(valuenode->Value());


                        dataitems[itemIndex].paramEx = ex;
                    }
                }break;
            case SerialType::DL64597:
                {
                    dataitems[itemIndex].paramEx = NULL;

                }break;
            default:
                break;
            }

        }


    }


    return 0;
}



ST_BOOLEAN StrToVariant(ST_VARIANT&, ST_INT16, ST_CHAR*);
ST_INT CCommManager::LoadGenerallyVariable (tinyxml2::XMLHandle handle)
{
    if (! handle.ToNode()) {
        printf("[Error] Not Found GenerallyVariable element.\n");
        return -1;
    }
    Rt_Init();
    ST_CHAR varname[256] = {0};
    tinyxml2::XMLNode* node = handle.FirstChildElement("Name").FirstChild().ToNode();
    if (node) strncpy(varname, node->Value(), 255);
    Rt_AddNameNode(varname);

    ST_Variable *pVarObj = new ST_Variable();
    memset (pVarObj, 0, sizeof(ST_Variable));
    pVarObj->id = time(0);

    Strcpy(pVarObj->name, varname);

//    node = handle.FirstChildElement("Description").FirstChild().ToNode();

    int vartype = VALType_Float;
    node = handle.FirstChildElement("Datatype").FirstChild().ToNode();
    if (node) vartype = atoi(node->Value());

    node = handle.FirstChildElement("Datalen").FirstChild().ToNode();
    if (node) pVarObj->datalen = atoi(node->Value());

    node = handle.FirstChildElement("Overlimit").FirstChild().ToNode();
    if (node) pVarObj->overlimit = atoi(node->Value());

    node = handle.FirstChildElement("Decimal").FirstChild().ToNode();
    if (node) pVarObj->decimal = atoi(node->Value());

    node = handle.FirstChildElement("Storagedistance").FirstChild().ToNode();
    if (node) pVarObj->storagedistance = atoi(node->Value());

    node = handle.FirstChildElement("Zero").FirstChild().ToNode();
    if (node) pVarObj->zero = atof(node->Value());

    node = handle.FirstChildElement("Offset").FirstChild().ToNode();
    if (node) pVarObj->offset = atof(node->Value());

    node = handle.FirstChildElement("Maxvaliedvalue").FirstChild().ToNode();
    if (node) pVarObj->maxvaliedvalue = atof(node->Value());

    node = handle.FirstChildElement("Minvaliedvalue").FirstChild().ToNode();
    if (node) pVarObj->minvaliedvalue = atof(node->Value());

    ST_CHAR varstr[32] = {0};
    node = handle.FirstChildElement("Prevalue").FirstChild().ToNode();
    if (node) strncpy(varstr, node->Value(), 32);

    ST_VARIANT var;
    StrToVariant (var,vartype,varstr);
    Strcat(varname,".value");
    Rt_AddValueNode(varname, vartype);

    node = handle.FirstChildElement("Coefficient").FirstChild().ToNode();
    if (node) {
        pVarObj->coefficient = atof(node->Value());
        if (pVarObj->coefficient == 0.0) pVarObj->coefficient = 1.0;
    }
//    node = handle.FirstChildElement("Attribute").FirstChild().ToNode();

//    node = handle.FirstChildElement("State").FirstChild().ToNode();

    ST_DUADDR duaddr;
    node = handle.FirstChildElement("Fromtype").FirstChild().ToNode();
    if (node) pVarObj->fromtype = atoi(node->Value());
    duaddr.type = pVarObj->fromtype;

    node = handle.FirstChildElement("Fromobject").FirstChild().ToNode();
    if (node) pVarObj->fromobject = atoi(node->Value());
    duaddr.device = pVarObj->fromobject;

    node = handle.FirstChildElement("FromItemaddr").FirstChild().ToNode();
    if (node) pVarObj->fromItemaddr = atoi(node->Value());
    duaddr.addr = pVarObj->fromItemaddr;

    node = handle.FirstChildElement("Fromconnect").FirstChild().ToNode();
    if (node) pVarObj->fromconnect = atoi(node->Value());
    duaddr.connect = pVarObj->fromconnect;

    Rt_SetNodeAddr(varname, duaddr);
//    node = handle.FirstChildElement("Access").FirstChild().ToNode();

    node = handle.FirstChildElement("Apptype").FirstChild().ToNode();
    if (node) pVarObj->apptype = atoi(node->Value());

    node = handle.FirstChildElement("Area").FirstChild().ToNode();
    if (node) pVarObj->area = atoi(node->Value());

    node = handle.FirstChildElement("Filtersavetype").FirstChild().ToNode();
    if (node) pVarObj->filtersavetype = atoi (node->Value());

    node = handle.FirstChildElement("Filtersaveminvalue").FirstChild().ToNode();
    if (node) pVarObj->filtersaveminvalue = atof (node->Value());

    node = handle.FirstChildElement("Filtersavemaxvalue").FirstChild().ToNode();
    if (node) pVarObj->filtersavemaxvalue = atof (node->Value());

    node = handle.FirstChildElement("Eventtype").FirstChild().ToNode();
    if (node) strncpy (pVarObj->eventtype, node->Value(), 12);


    Rt_SetNodeValue(varname,var);
    Rt_SetNodeVariable(varname, pVarObj);
    m_pSave->AddVariable(duaddr,var);
    return 0;
}

int LoadTransTable (tinyxml2::XMLHandle handle, TransferTable& table)
{
    if (!handle.ToNode()) {
        return -1;
    }

    std::map<long, tinyxml2::XMLNode*> sortctr;
    tinyxml2::XMLNode * node = handle.FirstChildElement("transtdatas"). // ! die
        FirstChildElement("transtdata").ToNode();
    for (; node; node = node->NextSibling()) {
        tinyxml2::XMLNode* tempnode = tinyxml2::XMLHandle(node).FirstChildElement("index").FirstChild().ToNode();
        long tempno = -1;
        if (tempnode) tempno = atoi(tempnode->Value());
        sortctr.insert (std::make_pair(tempno, node));
    }
    if (sortctr.empty())
        return -2;
    int _size = sortctr.size();

    // for (node = handle.FirstChildElement("transtdatas").FirstChildElement("transtdata").ToNode();
    //     node; node = node->NextSibling())
    for (int index = 0; index < _size; ++index) {
	    TRVariable * tvar = new TRVariable();
	    memset (tvar, 0, sizeof(TRVariable));

        std::map<long, tinyxml2::XMLNode*>::iterator it = sortctr.find (index);
        if (it != sortctr.end()) {
    	    tinyxml2::XMLHandle vhandle (it->second);
    	    tinyxml2::XMLNode *
    	    vnode = vhandle.FirstChildElement("fromchannel" ).FirstChild().ToNode();
    	    if (vnode) tvar->Addr.connect = atoi (vnode->Value());

    	    vnode = vhandle.FirstChildElement("fromdevice"  ).FirstChild().ToNode();
    	    if (vnode) tvar->Addr.device  = atoi (vnode->Value());

    	    vnode = vhandle.FirstChildElement("pointnum"    ).FirstChild().ToNode();
    	    if (vnode) tvar->Addr.addr    = atoi (vnode->Value());

            vnode = vhandle.FirstChildElement("fromtype"    ).FirstChild().ToNode();
            if (vnode) tvar->Addr.type    = atoi (vnode->Value());

    	    vnode = vhandle.FirstChildElement("cofficient"  ).FirstChild().ToNode();
    	    if (vnode) tvar->Coefficient  = atof (vnode->Value());

            vnode = vhandle.FirstChildElement("itemdatatype").FirstChild().ToNode();
    	    if (vnode) tvar->ItemType     = atoi (vnode->Value());

        }
	    table.AddTRVariable(&tvar->Addr, tvar);
    }
    return 0;
}

int LoadTransTableList (tinyxml2::XMLHandle handle, Communication& cmm)
{
    if (!handle.ToNode()) {
        return -1;
    }

    for (tinyxml2::XMLNode * node = handle.FirstChildElement("transtablelist").ToNode();
        node; node = node->NextSibling())
    {
        tinyxml2::XMLHandle lhandle (node);
        tinyxml2::XMLNode *
        ntemp = lhandle.FirstChildElement("channel").FirstChild().ToNode();
        if (!ntemp) continue;
        int chlid = atoi(ntemp->Value());
        ntemp = lhandle.FirstChildElement("device" ).FirstChild().ToNode();
        if (!ntemp) continue;
        int devid = atoi(ntemp->Value());

        Channel * chl = cmm.GetChannel (chlid);
        if (!chl)   continue;

        Device  * dev = chl->GetDevice (devid);
        if (!dev)   continue;

        for (tinyxml2::XMLNode * lnode = lhandle.FirstChildElement("transtables"). // ! die
        FirstChildElement("transtable").ToNode(); lnode; lnode = lnode->NextSibling())
        {
            int transtype = -1;
            tinyxml2::XMLNode * temp = tinyxml2::XMLHandle(lnode).FirstChildElement("no").FirstChild().ToNode();
            if (node) transtype = atoi (temp->Value());

            ST_CHAR tablename[64] = {0};
            snprintf (tablename, sizeof(tablename), "table %d %d %d",chlid, devid, transtype);

            TransferTable * table = dev->TransTables().GetTable(tablename);
            if(!table) {
                table = new TransferTable(8);
                Strcpy(table->TableName, tablename);
                table->TableType = transtype;
                dev->TransTables().AddTable(table);
            }
            LoadTransTable (lnode, *table);
        }
    }
    return 0;
}

ST_VOID CCommManager::LoadAllTransTableList()
{
    const char * trname1 = "/etc/comm/variabletransformer.xml";
    const char * trname2 = "./channelconfig/variabletransformer.xml";
    const char * trname0 = 0;

    if (access (trname2, F_OK) == 0) trname0 = trname2;
    if (access (trname1, F_OK) == 0) trname0 = trname1;

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile (trname0)) {
        return ;
    }

    tinyxml2::XMLHandle handle (&doc);

    tinyxml2::XMLNode * node = handle.FirstChildElement("ArrayOfTranstablelist").ToNode();

    LoadTransTableList (node, *m_pComm);
/*
    ST_INT transChannelId,transType;
    ST_INT transDeviceId,index;
    ST_CHAR tableName[64];
    TRVariable *tVariable;
	RecordSet *varset = m_pMySql->ExecQuery("select * from variabletransformer a order by a.channel,a.device,a.type,a.no");
	while(!varset->IsEof()){
	    ST_DUADDR duAddr;
	    tVariable=new TRVariable();
	    duAddr.type=0;
		if(varset->GetCurrentFieldValue("channel",11,fieldValue)){
			transChannelId =atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("device",11,fieldValue)){
			transDeviceId=atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("fromchannel",11,fieldValue)){
			duAddr.connect =atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("fromdevice",11,fieldValue)){
			duAddr.device=atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("index",11,fieldValue)){
			duAddr.addr=atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("type",11,fieldValue)){
			transType=atoi(fieldValue);
		}
        if(varset->GetCurrentFieldValue("no",11,fieldValue)){
			index=atoi(fieldValue);
		}
		if(varset->GetCurrentFieldValue("coefficient",16,fieldValue)){
			tVariable->Coefficient=atof(fieldValue);
		}
		tVariable->Addr=duAddr;
		sprintf(tableName,"table%d%d%d",transChannelId,transDeviceId,transType);
		List<Channel> *channels=m_pComm->GetChannels();

        for(int i=0;i<channels->GetCount();i++){
            Channel *channel=channels->GetItem(i);
            if(channel==NULL) continue;
            List<Device> *devices=channel->GetDevices();

            for(int j=0;j<devices->GetCount();j++){
                Device *device=devices->GetItem(j);
                if(device==NULL) continue;
                if(channel->GetChannelInfo()->ChannelID==transChannelId && device->GetDeviceInfo()->DeviceId==transDeviceId){
                    if(!device->m_transTable.IsExistTable(tableName)){
                        TransferTable *table=new TransferTable(8);
                        Strcpy(table->TableName,tableName);
                        table->AddTRVariable(&tVariable->Addr,tVariable);
                        device->m_transTable.AddTable(table);
 //                       printf("tableName = %s",tableName);
   //                     printf("tablecount = %d",device->m_transTable.m_pTables->GetCount());
                    }
                    else{
                        TransferTable *table=device->m_transTable.GetTable(tableName);
                        if(table!=NULL){
                            table->AddTRVariable(&tVariable->Addr,tVariable);
                        }
                    }
                }
            }
        }
		varset->MoveNext();
	}
	delete varset;*/
}

ST_BOOLEAN StrToVariant(ST_VARIANT &var, ST_INT16 vt, ST_CHAR* value)
{
    ST_CHAR *ptr;
    var.vt = vt;
    switch(vt) {
	case VALType_Byte:    var.bVal  = (ST_BYTE)   atoi(value);
		break;
	case VALType_Int16:   var.sVal  = (ST_INT16)  atoi(value);
		break;
	case VALType_Int32:   var.iVal  = (ST_INT32)  atol(value);
		break;
    case VALType_Int64:   var.lVal  = (ST_INT64)  atoll(value);
		break;
	case VALType_UInt16:  var.usVal = (ST_UINT16) atoi(value);
		break;
	case VALType_UInt32:  var.uiVal = (ST_UINT32) atol(value);
		break;
	case VALType_UInt64:  var.ulVal = (ST_UINT64) atoll(value);
		break;
	case VALType_Float:   var.fVal  = (ST_FLOAT)  atof(value);
		break;
	case VALType_Boolean: var.blVal = (ST_BOOLEAN)atoi(value);
		break;
    case VALType_Double:  var.dtVal = (ST_DOUBLE) strtod(value,&ptr);
		break;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_VOID CCommManager::Start()
{
    m_pWdog->Run();

    m_pComm->Init();

    LoadAllTransTableList();

    m_pComm->Work();

    m_pSave->Init();

    m_pSave->Start();

    m_MQTTUpload.Start();

    //m_server.Start();


    //m_historystorage.Start();

//    RtValueTransfer::Instance().Start();
}

ST_VOID CCommManager::Stop()
{
    m_pComm->Stop();
    m_pComm->Uninit();
    m_pSave->Stop();
    m_pSave->Uninit();

    //m_historystorage.Stop();
    //m_MQTTUpload.Stop();


//    RtValueTransfer::Instance().Stop();
}




