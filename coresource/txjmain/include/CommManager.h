
#ifndef CCOMMMANAGER_INCLUDE_H
#define CCOMMMANAGER_INCLUDE_H

#include "datatype.h"

#include <string>

#include <boost/scoped_ptr.hpp>

#include "tinyxml2.h"
#include "HistoryStorage.h"
#include "MQTTUpload.h"

using namespace std;

struct PortInfoA;
struct DeviceInfoA;
struct DataAreaItem;
struct DeviceDataArea;
// hhc new add
struct DataAreaItemType;
struct DataAreaItem_OPC;
struct DataAreaItem_64507;
struct DataAreaItem_64597;

struct DeviceDataAreaEx;
struct DataAreaItemEx;

class CCommManager
{
public:
	CCommManager();
	~CCommManager();

    ST_INT LoadConfigs ();

	ST_VOID LoadAllTransTableList();

	ST_VOID Start();
	ST_VOID Stop();
private:

    ST_INT LoadChannels (tinyxml2::XMLHandle, tinyxml2::XMLHandle);
    ST_INT LoadChannel  (tinyxml2::XMLHandle);
    PortInfoA* LoadPort (tinyxml2::XMLHandle);
    ST_INT LoadDevices  (tinyxml2::XMLHandle, tinyxml2::XMLHandle);
    ST_INT LoadDevice   (tinyxml2::XMLHandle, tinyxml2::XMLHandle, DeviceInfoA&);

    tinyxml2::XMLNode* FindDeviceTemplateSerial (tinyxml2::XMLHandle devHandle, tinyxml2::XMLHandle tmpHandle);
    DeviceDataArea* LoadDataAreas(tinyxml2::XMLHandle, int&, int&);
    ST_INT LoadDataArea (tinyxml2::XMLHandle, DeviceDataArea&, int&);
    DataAreaItem* LoadDataItems (tinyxml2::XMLHandle, int&, int&);
    ST_INT LoadDataItem (tinyxml2::XMLHandle, DataAreaItem*, int&);
    ST_INT LoadGenerallyVariable (tinyxml2::XMLHandle);
    ST_INT LoadExDataAreas(tinyxml2::XMLHandle,DeviceDataAreaEx *, int, int);


private:
	class Communication	  *m_pComm;
	class VariableStorage *m_pSave;
    class WatchdogHelper  *m_pWdog;
    HistoryStorage   m_historystorage;
    MQTTUpload m_MQTTUpload;

};


#endif //CCOMMMANAGER_INCLUDE_H
