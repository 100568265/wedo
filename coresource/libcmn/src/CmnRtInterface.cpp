//#include "stdafx.h"
#include "CmnRtInterface.h"
#include "Channel.h"
#include "Device.h"


ST_BOOLEAN CmnRtInterface::g_Inited = false;

CmnRtInterface::CmnRtInterface()
{

}

CmnRtInterface::~CmnRtInterface()
{

}

ST_VOID CmnRtInterface::Init()
{
    CmnRtInterface::g_Inited=true;
}

ST_VOID CmnRtInterface::UnInit()
{
    CmnRtInterface::g_Inited=false;
}

ST_VOID CmnRtInterface::OnVariableChange(ST_CHAR *vname,ST_VARIANT var)
{

}

ST_INT CmnRtInterface::GetVariableValueByName(ST_CHAR *fullName,ST_VARIANT &var)
{
    return Rt_GetNodeValue(fullName, &var);
}

ST_INT CmnRtInterface::GetVariableValueByAddr(ST_DUADDR addr,ST_VARIANT &var)
{
    return Rt_GetNodeValueByAddr(&addr, &var);
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_BYTE value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Byte;
	    vVar.bVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT32 value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_UInt32;
	    vVar.uiVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT64 value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_UInt64;
	    vVar.ulVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Byte;
	    vVar.cVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT16 value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Int16;
	    vVar.sVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Int32;
	    vVar.iVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_FLOAT value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Float;
	    vVar.fVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DOUBLE value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_Double;
	    vVar.dVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR* value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	if(retVal==SD_SUCCESS){
	    ST_VARIANT vVar;
	    vVar.vt=VALType_String;
	    vVar.strVal=value;
        retVal=Rt_SetNodeValue(varName,vVar);
	}
	return retVal;
}

ST_INT CmnRtInterface::UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DECIMAL value)
{
    ST_CHAR varName[512];
    ST_INT retVal;
	ST_DUADDR duAddr;
	duAddr.type=channelType;
	duAddr.connect = channelId;
	duAddr.device =deviceId;
	duAddr.addr = addrNo;
	retVal=Rt_GetNodeNameByAddr(&duAddr, varName);
	return SD_FAILURE;
}

//List<ST_Variable> *CmnRtInterface::GetVariables(ST_CHAR *fullName,ST_INT addr)
//{
//	return Rt_GetVariables(fullName,addr);
//}

ST_VOID CmnRtInterface::SetVariableValue(ST_CHAR *fullName,ST_VARIANT& var)
{
	Rt_SetNodeValue(fullName,var);
}

ST_VOID SetVariableValue(ST_CHAR *fullName,ST_VARIANT var)
{
	CmnRtInterface::SetVariableValue(fullName,var);

}

ST_VOID UpdateByteValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_BYTE value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateWordValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT32 value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateDwordValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT64 value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateCharValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateShortValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT16 value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateIntValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateFloatValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_FLOAT value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateDoubleValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DOUBLE value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateStrValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR* value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

ST_VOID UpdateDecimalalue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DECIMAL value)
{
    CmnRtInterface::UpdateValue(channelType,channelId,deviceId,addrNo,value);
}

//List<ST_Variable> *GetVariables(ST_CHAR *fullName,ST_INT addr)
//{
//	return CmnRtInterface::GetVariables(fullName,addr);
//}

ST_INT GetVariableValueByName(ST_CHAR *fullName,ST_VARIANT &var)
{
    return CmnRtInterface::GetVariableValueByName(fullName,var);
}

ST_INT GetVariableValueByAddr(ST_DUADDR addr,ST_VARIANT &var)
{
    return CmnRtInterface::GetVariableValueByAddr(addr,var);
}
