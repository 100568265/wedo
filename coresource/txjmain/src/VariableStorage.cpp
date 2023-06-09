#include <math.h>
#include "VariableStorage.h"
#include "syslogger.h"

#define SYSLOGGER SysLogger::GetInstance()

VariableStorage::VariableStorage():
run(false),
init(false),
exit(false)
{
}

VariableStorage::~VariableStorage()
{
//    if(m_pMySql!=NULL){
//        delete m_pMySql;
//        m_pMySql=NULL;
//    }
}
/*
ST_INT VariableStorage::ConnectDatabase()
{
	int ret=0;
	CIniFile iniFile(".//Config//DBConfig.ini");
	ret=iniFile.OpenIni();
	if(ret==0) return ret;
	serverName=iniFile.GetValue("DBConfig","ServerName");
	string port=iniFile.GetValue("DBConfig","ServerPort");
	serverPort=atoi(port.c_str());
	dbName=iniFile.GetValue("DBConfig","DatabaseName");
	userName=iniFile.GetValue("DBConfig","UserName");
	passWord=iniFile.GetValue("DBConfig","Password");
    iniFile.CloseIni();
	ret= m_pMySql->Connect(serverName,userName,passWord,dbName,serverPort,0);
	return ret;
}
*/

ST_VOID VariableStorage::AddVariable(ST_DUADDR addr,ST_VARIANT value)
{
    ST_SaveOjbect saveObj;
    saveObj.duAddr=addr;
    saveObj.value=value;
    savelist.push_back(saveObj);
}


ST_BOOLEAN GetValue(const ST_VARIANT& var,ST_DOUBLE &preValue)
{
    preValue = 0;
    switch(var.vt) {
	case VALType_Byte:
		preValue = var.bVal;
		break;
	case VALType_Int16:
		preValue = var.sVal;
		break;
	case VALType_Int32:
		preValue = var.iVal;
		break;
    case VALType_Int64:
		preValue = var.lVal;
		break;
	case VALType_UInt16:
		preValue = var.usVal;
		break;
	case VALType_UInt32:
		preValue = var.uiVal;
		break;
	case VALType_UInt64:
		preValue = var.ulVal;
		break;
	case VALType_Float:
		preValue = var.fVal;
		break;
	case VALType_Boolean:
		preValue = var.blVal ;
		break;
    case VALType_Double:
		preValue = var.dtVal ;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_VOID VariableStorage::Init()
{
//    if(ConnectDatabase()) return;
    exit = false;
    init = true;
}

ST_VOID VariableStorage::Uninit()
{
    exit = true;
    savelist.clear();
}

ST_VOID VariableStorage::Start()
{
    if(!init) return;
    run = true;
    m_Thread.Start(SaveProc, this, false);
}

ST_VOID VariableStorage::Stop()
{
    run = false;
    m_Thread.Stop();
}


ST_VOID VariableStorage::Save()
{

    std::string text;

    ST_DOUBLE prevalue = 0.0;
    ST_DOUBLE prepreValue[40960] = {0};
    ST_INT ni = 0;
    while (!exit) {
        if (!run) continue;
        ni = 0;
        for(vector<SaveObject>::iterator it = savelist.begin(); it != savelist.end(); it++)
        {
            SaveObject so = (SaveObject)(*it);
            Rt_GetNodeValueByAddr(&so.duAddr, &so.value);
            GetValue(so.value,prevalue);
            if(fabs(prevalue - prepreValue[ni]) > 0.001) {
                prepreValue[ni++] = prevalue;
            }
            Thread::SLEEP(30);
        }
    }

}

ST_VOID *VariableStorage::SaveProc(ST_VOID *param)
{
	    try {

    VariableStorage *vs = (VariableStorage*)param;
    vs->Save();}
        catch (std::exception e) {
    	SYSLOGGER->LogWarn("Historical data storage exception. Msg:%s", e.what());
    }
	return 0;
}

