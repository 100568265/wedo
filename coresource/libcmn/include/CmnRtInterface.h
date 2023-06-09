#ifndef CMNRTINTERFACE_H
#define CMNRTINTERFACE_H

#include "rtbase.h"
#include "datatype.h"

#ifdef __cplusplus
extern "C"
{
#endif


typedef ST_VOID (TXJ_STDCALL *VariableChangeBack)(ST_CHAR *vname,ST_VARIANT var);

class Device;
class Channel;

class CmnRtInterface
{
public:
    CmnRtInterface();
    virtual ~CmnRtInterface();
    static ST_VOID Init();
	static ST_VOID UnInit();
	static ST_VOID OnVariableChange(ST_CHAR *vname,ST_VARIANT var);
    static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_BYTE    value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT32  value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT64  value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR    value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT16   value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT     value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_FLOAT   value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DOUBLE  value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR*   value);
	static ST_INT UpdateValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DECIMAL value);
//	static List<ST_Variable> *GetVariables(ST_CHAR *fullName,ST_INT addr);
	static ST_VOID SetVariableValue(ST_CHAR *fullName,ST_VARIANT& var);
	static ST_INT GetVariableValueByName(ST_CHAR *fullName,ST_VARIANT &var);
	static ST_INT GetVariableValueByAddr(ST_DUADDR addr,ST_VARIANT &var);
public:
	static ST_BOOLEAN g_Inited;
	static VariableChangeBack g_pVariableChangeBack;
};



extern ST_VOID UpdateByteValue  (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_BYTE    value);
extern ST_VOID UpdateWordValue  (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT32  value);
extern ST_VOID UpdateDwordValue (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_UINT64  value);
extern ST_VOID UpdateCharValue  (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR    value);
extern ST_VOID UpdateShortValue (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT16   value);
extern ST_VOID UpdateIntValue   (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_INT     value);
extern ST_VOID UpdateFloatValue (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_FLOAT   value);
extern ST_VOID UpdateDoubleValue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DOUBLE  value);
extern ST_VOID UpdateStrValue   (ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_CHAR*   value);
extern ST_VOID UpdateDecimalalue(ST_INT channelType,ST_INT channelId,ST_INT deviceId,ST_INT addrNo,ST_DECIMAL value);
//extern List<ST_Variable> *GetVariables(ST_CHAR *fullName,ST_INT addr);
extern ST_VOID SetVariableValue(ST_CHAR *fullName,ST_VARIANT var);
extern ST_INT GetVariableValueByName(ST_CHAR *fullName,ST_VARIANT &var);
extern ST_INT GetVariableValueByAddr(ST_DUADDR addr,ST_VARIANT &var);

#ifdef __cplusplus
}
#endif
#endif // CMNRTINTERFACE_H
