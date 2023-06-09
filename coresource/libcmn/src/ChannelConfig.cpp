//#include "stdafx.h"
#include "ChannelConfig.h"

ChannelConfig::ChannelConfig()
{
	channelInfos=NULL;
    m_pLogger=SysLogger::GetInstance();
}

ChannelConfig::~ChannelConfig()
{

}

List<ChannelInfo> *ChannelConfig::LoadChannelConfig()
{
    return NULL;
/*    //const ST_CHAR * xmlFile = "D:/txjcode/build/txj_windows/channelconfig/channelconfig.xml";
	const ST_CHAR * xmlFile = "./channelconfig/channelconfig.xml";
    ST_INT portType;
    ST_INT intValue;
    TiXmlDocument doc;
    if (doc.LoadFile(xmlFile))
    {
        channelInfos=new List<ChannelInfo>();
        m_pLogger->LogDebug("load ChannelConfig Success!");
        TiXmlElement *rootElement = doc.RootElement();
        TiXmlElement *channelElement = rootElement->FirstChildElement();

        for(;channelElement!=NULL;channelElement=channelElement->NextSiblingElement())
        {
            ChannelInfo *m_pChannelInfo=new ChannelInfo();
            #ifdef _WIN32
            strcpy_s(m_pChannelInfo->ChannelName,65,channelElement->Attribute("name"));
            #else
            snprintf(m_pChannelInfo->ChannelName,65,channelElement->Attribute("name"));
            #endif
            channelElement->Attribute("channelId",&intValue);
            m_pChannelInfo->ChannelID=intValue;

            channelElement->Attribute("transChannelID",&intValue);
            m_pChannelInfo->TransChannelID=intValue;

            channelElement->Attribute("channelInterval",&intValue);
            m_pChannelInfo->ChannelInterval=intValue;

            channelElement->Attribute("protocolType",&intValue);
            m_pChannelInfo->ProtocolType=intValue;

            channelElement->Attribute("autoOpen",&intValue);
            m_pChannelInfo->AutoOpen=intValue;

			channelElement->Attribute("protocolType",&intValue);
			m_pChannelInfo->ProtocolType=intValue;

			channelElement->Attribute("maxConnects",&intValue);
			if(intValue<=0)intValue=1;
			if(intValue>100) intValue=100;
			m_pChannelInfo->MaxConnects=intValue;

			 #ifdef _WIN32
			strcpy_s(m_pChannelInfo->ProtocolFile,65,channelElement->Attribute("protocolFile"));
            #else
            snprintf(m_pChannelInfo->ProtocolFile,65,channelElement->Attribute("protocolFile"));
            #endif


            TiXmlElement *portsElement = channelElement->FirstChildElement("Ports");
            TiXmlElement *mainPort = portsElement->FirstChildElement("MainPort");
            if(NULL==mainPort) goto next1;
            m_pChannelInfo->MainPort=new PortInfo();
            mainPort->Attribute("portType",&intValue);
            portType=intValue;
            switch(portType)
            {
                case 1: //com
                {
                    m_pChannelInfo->MainPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #endif

                    mainPort->Attribute("portNum",&intValue);
                    m_pChannelInfo->MainPort->PortNum=intValue;
                    mainPort->Attribute("baudRate",&intValue);
                    m_pChannelInfo->MainPort->BaudRate=intValue;
                    mainPort->Attribute("dataBits",&intValue);
                    m_pChannelInfo->MainPort->DataBits=intValue;
                    mainPort->Attribute("stopBits",&intValue);
                    m_pChannelInfo->MainPort->StopBits=intValue;
                    mainPort->Attribute("parity",&intValue);
                    m_pChannelInfo->MainPort->Parity=intValue;
                    break;
                }
                case 2: //tcpserver
                {
                    m_pChannelInfo->MainPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #endif

                    mainPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->MainPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #endif
                    break;
                }
                case 3: //tcpclient
                {
                    m_pChannelInfo->MainPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #endif

                    mainPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->MainPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #endif

                    mainPort->Attribute("remotePort",&intValue);
                    m_pChannelInfo->MainPort->RemotePort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->RemoteAddress,65,mainPort->Attribute("remoteAddress"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->RemoteAddress,65,mainPort->Attribute("remoteAddress"));
                    #endif

                    break;
                }
                case 4: //udp
                {
                    m_pChannelInfo->MainPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->PortName,65,mainPort->Attribute("name"));
                    #endif

                    mainPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->MainPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->LocalAddress,65,mainPort->Attribute("localAddress"));
                    #endif

                    mainPort->Attribute("remotePort",&intValue);
                    m_pChannelInfo->MainPort->RemotePort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->MainPort->RemoteAddress,65,mainPort->Attribute("remoteAddress"));
                    #else
                    snprintf(m_pChannelInfo->MainPort->RemoteAddress,65,mainPort->Attribute("remoteAddress"));
                    #endif

                    break;
                }
            }
    next1:
            TiXmlElement *slavPort = portsElement->FirstChildElement("SlavPort");
            if(NULL==slavPort) goto next2;
            m_pChannelInfo->SlavPort=new PortInfo();
            slavPort->Attribute("portType",&intValue);
            portType=intValue;
            switch(portType)
            {
                case 1: //com
                {
                    m_pChannelInfo->SlavPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #endif

                    slavPort->Attribute("portNum",&intValue);
                    m_pChannelInfo->SlavPort->PortNum=intValue;
                    slavPort->Attribute("baudRate",&intValue);
                    m_pChannelInfo->SlavPort->BaudRate=intValue;
                    slavPort->Attribute("dataBits",&intValue);
                    m_pChannelInfo->SlavPort->DataBits=intValue;
                    slavPort->Attribute("stopBits",&intValue);
                    m_pChannelInfo->SlavPort->StopBits=intValue;
                    slavPort->Attribute("parity",&intValue);
                    m_pChannelInfo->SlavPort->Parity=intValue;
                    break;
                }
                case 2: //tcpserver
                {
                    m_pChannelInfo->SlavPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #endif

                    slavPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->SlavPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #endif

                    break;
                }
                case 3: //tcpclient
                {
                    m_pChannelInfo->SlavPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #endif
                    slavPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->SlavPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #endif
                    slavPort->Attribute("remotePort",&intValue);
                    m_pChannelInfo->SlavPort->RemotePort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->RemoteAddress,65,slavPort->Attribute("remoteAddress"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->RemoteAddress,65,slavPort->Attribute("remoteAddress"));
                    #endif
                    break;
                }
                case 4: //udp
                {
                    m_pChannelInfo->SlavPort->PortType=portType;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->PortName,65,slavPort->Attribute("name"));
                    #endif
                    slavPort->Attribute("localPort",&intValue);
                    m_pChannelInfo->SlavPort->LocalPort=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->LocalAddress,65,slavPort->Attribute("localAddress"));
                    #endif
                    slavPort->Attribute("remotePort",&intValue);
                    m_pChannelInfo->SlavPort->RemotePort=intValue;
                    slavPort->Attribute("isMulticast",&intValue);
                    m_pChannelInfo->SlavPort->Multicast=intValue;
                    #ifdef _WIN32
                    strcpy_s(m_pChannelInfo->SlavPort->RemoteAddress,65,slavPort->Attribute("remoteAddress"));
                    #else
                    snprintf(m_pChannelInfo->SlavPort->RemoteAddress,65,slavPort->Attribute("remoteAddress"));
                    #endif
                    break;
                }
            }

    next2:
            TiXmlElement *devicesElement=channelElement->FirstChildElement("Devices");
            TiXmlElement *deviceElement=devicesElement->FirstChildElement();
            m_pChannelInfo->DeviceInfos=new List<DeviceInfo>();
            for(;deviceElement!=NULL;deviceElement=devicesElement->NextSiblingElement())
            {
                DeviceInfo *deviceInfo=new DeviceInfo();
                #ifdef _WIN32
                strcpy_s(deviceInfo->DeviceName,65,deviceElement->Attribute("name"));
                #else
                snprintf(deviceInfo->DeviceName,65,deviceElement->Attribute("name"));
                #endif

                deviceElement->Attribute("address",&intValue);
                deviceInfo->Address=intValue;
                #ifdef _WIN32
                strcpy_s(deviceInfo->ProtocolFile,255,deviceElement->Attribute("protocolFile"));
                #else
                snprintf(deviceInfo->ProtocolFile,255,deviceElement->Attribute("protocolFile"));
                #endif

                deviceElement->Attribute("resend",&intValue);
                deviceInfo->ReSend=intValue;
                deviceElement->Attribute("break",&intValue);
                deviceInfo->Break=intValue;
                deviceElement->Attribute("isRun",&intValue);
                deviceInfo->IsRun=intValue;
                deviceElement->Attribute("deviceId",&intValue);
                deviceInfo->DeviceId=intValue;
				deviceElement->Attribute("transDeviceId",&intValue);
				deviceInfo->transDeviceId=intValue;
                m_pChannelInfo->DeviceInfos->Add(deviceInfo);
            }
            channelInfos->Add(m_pChannelInfo);
        }
    }
    return channelInfos;*/
}
