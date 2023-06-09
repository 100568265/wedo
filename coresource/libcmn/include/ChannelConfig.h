#ifndef CHANNELCONFIG_H
#define CHANNELCONFIG_H

#include "sysgenlist.h"
#include "datatype.h"
#include "sysstring.h"
#include "syslogger.h"

#include <string.h>


//
struct SerialType
{
    enum
    {
        DEFAULT = 0,
        DL64507 = 1,
        DL64597 = 2,
        OPCUA   = 3
    };
};


class IParamEx
{
public:
    IParamEx() {}
    virtual ~IParamEx() {}
};

class DeviceInfoEx_DL64507 : public IParamEx
{
public:
    char Passwd[6];
    char Ctrl[8];
    char Date[12];
};

class DataAreaItemEx_64507 : public IParamEx
{
public:
    // XML 传入的时 DI3-DI0 这里也时 DI3-DI0
    ST_UINT32 dataid;
    ST_INT parsetype;
    ST_BYTE kmstate; // 也许用不到
};


class DataAreaItemEx_64597 : public IParamEx
{
public:
    ST_UINT16 dataid;
};


class DataAreaItemEx_OPCUA : public IParamEx
{
public:
    std::string  nodeid;
};



typedef struct DataAreaItem
{
	ST_CHAR  itemName[64];
	ST_INT   dataType;
	ST_INT   dataLen;
	ST_INT   addr;
	ST_INT   beginBit;
	ST_INT   endBit;
	ST_INT   id;
	ST_INT   codeType;
	ST_FLOAT coeficient;
	ST_INT   reserver0;
	ST_INT   reserver1;
/*
    ST_INT   destOffset;
	ST_INT   paramExType;
    IParamEx* paramEx;*/
}ST_DataAreaItem;


typedef struct DeviceDataArea
{
	ST_CHAR areaName[64];
	ST_INT  addr;
	ST_INT  len;
	ST_INT  itemCount;
	ST_INT  readCode;
	ST_INT  writeCode;
	ST_INT  dataUnitLen;
	ST_INT  reserver0;
	ST_INT  reserver1;
	ST_DataAreaItem* items;
/*
	ST_INT  paramExType;
	IParamEx* paramEx;*/
}ST_DeviceDataArea;

typedef struct DataAreaItemEx
{
    DataAreaItemEx(): paramExType(0), paramEx(NULL) { }
    ~DataAreaItemEx()
    {
        if (paramEx)
            delete paramEx;
    }
    ST_INT   destOffset;
    ST_INT   paramExType;
    IParamEx* paramEx;
}ST_DataAreaItemEx;

typedef struct DeviceDataAreaEx
{
    DeviceDataAreaEx(): paramExType(0), paramEx(NULL) {}
    ~DeviceDataAreaEx()
    {
        if (paramEx)
            delete paramEx;
    }
    ST_DataAreaItemEx* items;
    ST_INT  paramExType;
	IParamEx* paramEx;
}ST_DeviceDataAreaEx;

class PortInfo
{
public:
    PortInfo()
    {
        PortType=-1;
		PortNum=-1;
        *PortName='\0';
        LocalPort=-1;
        RemotePort=-1;
        *LocalAddress='\0';
        *RemoteAddress='\0';
        BaudRate=9600;
        DataBits=8;
        StopBits=1;
        Parity=0;
        Multicast=0;
        IsRemoteCtrl=0;

        *client_id='\0';
        *pub_topic='\0';
        *user='\0';
 		*passwd='\0';
    }

	PortInfo(const PortInfo &port) {
        memcpy (this, &port, sizeof(class PortInfo));
    }
    ~PortInfo() {}

    ST_INT		PortType;
    ST_CHAR		PortName[65];

    ST_INT		LocalPort;
    ST_CHAR		LocalAddress[65];
    ST_INT		RemotePort;
    ST_CHAR		RemoteAddress[65];
    ST_INT		Multicast;
    ST_INT      IsRemoteCtrl;

    ST_INT		PortNum;
    ST_INT		BaudRate;
    ST_INT		DataBits;
    ST_INT		StopBits;
    ST_INT		Parity;

    /*<client_id>123</client_id>
      <pub_topic>mailian/sigalarm</pub_topic>
      <user>zywl</user>
      <passwd>zywl123</passwd>*/
    ST_CHAR     client_id[65];
    ST_CHAR		pub_topic[65];
    ST_CHAR		user[65];
    ST_CHAR		passwd[65];

};

class DeviceInfo
{
public:
    DeviceInfo() {
        memset (this, 0, sizeof(class DeviceInfo));
        Address  = -1;
        ReSend   =  3;
        Break    =  1;
        DeviceId = -1;
        Channel  = -1;
        ParamEx  = NULL;
    }
    ~DeviceInfo()
    {
        if (ParamEx)
           delete ParamEx;
    }

    ST_CHAR			DeviceName[65];
    ST_INT			Address;
    ST_CHAR			ProtocolFile[256];
    ST_INT			ReSend;
    ST_INT			Break;
    ST_INT			IsRun;
    ST_INT			DeviceId;
	ST_INT			transDeviceId;
	ST_VOID			*Protocol;
	ST_CHAR			ProtocolName[256];
	ST_INT			ParentArea;
	ST_CHAR			Addressex[256];
	ST_INT			deviceaddr1;
	ST_INT			Channel;
	ST_CHAR			DeviceTypeID[256];
	ST_CHAR			Deviceserialtype[256];
	ST_INT			DataAreasCount;
	ST_DeviceDataArea *DataAreas;
    ST_DeviceDataAreaEx *ExDataAreas;

	ST_INT          ParamExType;
    IParamEx*       ParamEx;
};


class ChannelInfo
{
public:
    ChannelInfo() {
        memset (this, 0, sizeof(class ChannelInfo));
        ProtocolType    =  1;
        ChannelID       = -1;
        TransChannelID  = -1;
        ChannelInterval = -1;
		MaxConnects     =  1;
    }
    ~ChannelInfo() {
        if(NULL != MainPort) {
            delete MainPort;
            MainPort = NULL;
        }
        if(NULL != SlavPort) {
            delete SlavPort;
            SlavPort = NULL;
        }
        if(NULL != DeviceInfos) {
            // DeviceInfos->Clear();
            delete DeviceInfos;
            DeviceInfos = NULL;
        }
    }

    ST_CHAR				ChannelName[65];
	ST_CHAR				ProtocolFile[256];
	ST_CHAR				CtlProtocolFile[256];
    ST_BYTE				ProtocolType;
    ST_INT				ChannelID;
    ST_INT				TransChannelID;
    ST_INT				ChannelInterval;
    ST_INT				AutoOpen;
	ST_INT              MaxResendTimes;
	ST_INT				MaxConnects;
	ST_BOOLEAN          EnableTransfer;
    PortInfo			*MainPort;
    PortInfo			*SlavPort;
    List<DeviceInfo>	*DeviceInfos;
};

struct PortInfoA
{
    ST_INT		PortType;
    ST_CHAR		PortName[65];

    ST_INT		LocalPort;
    ST_CHAR		LocalAddress[65];
    ST_INT		RemotePort;
    ST_CHAR		RemoteAddress[65];
    ST_INT		Multicast;
    ST_INT      IsRemoteCtrl;

    ST_INT		PortNum;
    ST_INT		BaudRate;
    ST_INT		DataBits;
    ST_INT		StopBits;
    ST_INT		Parity;

    ST_CHAR     client_id[65];
    ST_CHAR		pub_topic[65];
    ST_CHAR		user[65];
    ST_CHAR		passwd[65];
};

struct DeviceInfoA
{
    ST_CHAR		DeviceName[65];
    ST_INT		Address;
    ST_CHAR		ProtocolFile[256];
	ST_CHAR		ProtocolName[256];
	ST_INT		ParentArea;
    ST_INT		ReSend;
    ST_INT		Break;
    ST_INT		IsRun;
    ST_INT		DeviceId;
	ST_INT		transDeviceId;
	ST_CHAR     Addressex[256];
	ST_INT		deviceaddr1;
	ST_INT		Channel;
	ST_CHAR		DeviceTypeID[256];
	ST_CHAR		Deviceserialtype[256];
	ST_INT		DataAreasCount;
	ST_DeviceDataArea *DataAreas;

    ST_DeviceDataAreaEx *ExDataAreas;  //lxb
	// hhc new add
	// template serial type
	// 生成的时候临时用
	ST_INT      SerialType;

    // hhac new add
	ST_INT      ParamExType;
    IParamEx*   ParamEx;  // no free
};

struct ChannelInfoA
{
    ST_CHAR		ChannelName[65];
	ST_CHAR		ProtocolFile[256];
	ST_CHAR		CtlProtocolFile[256];
    ST_BYTE		ProtocolType;
    ST_INT		ChannelID;
    ST_INT		TransChannelID;
    ST_INT		ChannelInterval;
    ST_INT		AutoOpen;
	ST_INT      MaxResendTimes;
	ST_INT		MaxConnects;
	ST_BOOLEAN  EnableTransfer;
};

class ChannelConfig
{
    public:
        ChannelConfig();
        virtual ~ChannelConfig();
        virtual List<ChannelInfo>	*LoadChannelConfig();
    protected:
        List<ChannelInfo>			*channelInfos;
        SysLogger					*m_pLogger;
};

#endif // CHANNELCONFIG_H
