#include "SaveObject.h"

VariableSave::VariableSave()
{
    m_pMySql=new DBMySql();
    savelist.clear();
    exit=false;
    init=false;
}

VariableSave::~VariableSave()
{
    if(m_pMySql!=NULL){
        delete m_pMySql;
        m_pMySql=NULL;
    }
}

ST_INT VariableSave::ConnectDatabase()
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

ST_VOID VariableSave::AddVariable(ST_DUADDR addr,ST_VARIANT value)
{
    ST_SaveOjbect saveObj;
    saveObj.duAddr=addr;
    saveObj.value=value;
    savelist.push_back(saveObj);
}


ST_BOOLEAN VariableSave::GetValue(ST_VARIANT var,ST_DOUBLE &preValue)
{
    preValue=0;
    switch(var.vt)
	{
	case VALType_Byte:
		preValue=var.bVal;
		break;
	case VALType_Int16:
		preValue=var.sVal;
		break;
	case VALType_Int32:
		preValue=var.iVal;
		break;
    case VALType_Int64:
		preValue=var.lVal;
		break;
	case VALType_UInt16:
		preValue=var.usVal;
		break;
	case VALType_UInt32:
		preValue=var.uiVal;
		break;
	case VALType_UInt64:
		preValue=var.ulVal;
		break;
	case VALType_Float:
		preValue=var.fVal;
		break;
	case VALType_Boolean:
		preValue=var.blVal ;
		break;
    case VALType_Double:
		preValue=var.dtVal ;
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

ST_VOID VariableSave::Init()
{
    if(ConnectDatabase()) return;
    exit=false;
    init=true;
}

ST_VOID VariableSave::Uninit()
{
    exit = true;
    savelist.clear();
}

ST_VOID VariableSave::Start()
{
    if(!init) return;
    run=true;
    m_Thread.Start(SaveProc,this,false);
}

ST_VOID VariableSave::Stop()
{
    run=false;
    m_Thread.Stop();
}


ST_VOID VariableSave::Save()
{
    try{
        ST_DOUBLE prevalue;
        while (!exit){
            Thread::SLEEP(5*1000);
            if (!run) continue;
            for(vector<SaveObject>::iterator it=savelist.begin();it!=savelist.end();it++){
                SaveObject so=(SaveObject)(*it);
                Rt_GetNodeValueByAddr(so.duAddr,so.value);
                GetValue(so.value,prevalue);
                sprintf(strSQL,"update variable set prevalue=%ld where fromconnect=%d and fromobject=%d and fromitemaddr=%d",prevalue,so.duAddr.connect,so.duAddr.device,so.duAddr.addr);
                m_pMySql->ExecNonQuery(strSQL);
                Thread::SLEEP(500);
            }
        }
    }
    catch (...){
    }
}

ST_VOID *VariableSave::SaveProc(ST_VOID *param)
{
    VariableSave *vs=(VariableSave*)param;
    vs->Save();
	return 0;
}
