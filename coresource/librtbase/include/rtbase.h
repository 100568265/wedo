#ifndef RTBASE_H
#define RTBASE_H

#include "datatype.h"
#include "rtobject.h"

#ifdef _WIN32
#include <oaidl.h>
#define RTBASEDLL_EXPORTS

#ifdef RTBASEDLL_EXPORTS
#define RTBASE_API __declspec(dllexport)
#else
#define RTBASE_API __declspec(dllimport)
#endif
#ifdef __cplusplus
extern "C"
{
#endif

RTBASE_API ST_INT Rt_Init();
RTBASE_API ST_INT Rt_UnInit();

RTBASE_API ST_INT Rt_GetNameNodeCount(const ST_CHAR * fullName);
RTBASE_API ST_INT Rt_GetNodeName     (const ST_CHAR * fullName, ST_INT index, ST_CHAR * name);
RTBASE_API ST_INT Rt_GetNodeNameByAddr(const ST_DUADDR * addr, ST_CHAR * name);

RTBASE_API ST_INT Rt_AddNameNode (const ST_CHAR * fullName);
RTBASE_API ST_INT Rt_AddValueNode(const ST_CHAR * fullName, ST_INT type);
RTBASE_API ST_INT Rt_RemoveNode  (const ST_CHAR * fullName);
//RTBASE_API List<ST_Variable> *Rt_GetVariables(ST_CHAR *fullName,ST_INT addr);
#ifdef _WIN32
RTBASE_API ST_INT Rt_GetNodeValueA(const ST_CHAR * fullName, VARIANT * var);
RTBASE_API ST_INT Rt_SetNodeValueA(const ST_CHAR * fullName, VARIANT   var);
RTBASE_API ST_INT Rt_RegistValueChangedA(NodeValueChangedA valueChangedback);
#endif
RTBASE_API ST_INT Rt_SetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT   var);
RTBASE_API ST_INT Rt_GetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT * var);

RTBASE_API ST_INT Rt_GetNodeValueType(const ST_CHAR * fullName, ST_INT * valType);

RTBASE_API ST_INT Rt_GetNodeValue(const ST_CHAR * fullName, ST_VARIANT * var);
RTBASE_API ST_INT Rt_SetNodeValue(const ST_CHAR * fullName, ST_VARIANT   var);

RTBASE_API ST_INT Rt_SetNodeVariable(const ST_CHAR * fullName, ST_Variable * pObj);

RTBASE_API ST_INT Rt_SetNodeAddr(const ST_CHAR * fullName, ST_DUADDR   addr);
RTBASE_API ST_INT Rt_GetNodeAddr(const ST_CHAR * fullName, ST_DUADDR * addr);

RTBASE_API ST_INT Rt_RegistValueChanged(NodeValueChanged valueChangedback);
RTBASE_API ST_INT Rt_UnRegistValueChanged();

#ifdef __cplusplus
}
#endif

#else

#ifdef __cplusplus
extern "C"
{
#endif

ST_INT Rt_Init();
ST_INT Rt_UnInit();

ST_INT Rt_GetNameNodeCount (const ST_CHAR * fullName);
ST_INT Rt_GetNodeName      (const ST_CHAR * fullName, ST_INT index, ST_CHAR * name);
ST_INT Rt_GetNodeNameByAddr(const ST_DUADDR * addr, ST_CHAR * name);

ST_INT Rt_AddNameNode (const ST_CHAR * fullName);
ST_INT Rt_AddValueNode(const ST_CHAR * fullName, ST_INT type);
ST_INT Rt_RemoveNode  (const ST_CHAR * fullName);

ST_INT Rt_GetNodeValue(const ST_CHAR * fullName, ST_VARIANT * var);
ST_INT Rt_SetNodeValue(const ST_CHAR * fullName, ST_VARIANT   var);

ST_INT Rt_GetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT * var);
ST_INT Rt_SetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT   var);

ST_INT Rt_SetNodeAddr(const ST_CHAR * fullName, ST_DUADDR   addr);
ST_INT Rt_GetNodeAddr(const ST_CHAR * fullName, ST_DUADDR * addr);

ST_INT Rt_RegistValueChanged(NodeValueChanged valueChangedback);
ST_INT Rt_UnRegistValueChanged();

ST_INT Rt_SetNodeVariable(const ST_CHAR * fullName, ST_Variable * pObj);
//List<ST_Variable> *Rt_GetVariables(ST_CHAR *fullName,ST_INT addr);
#ifdef __cplusplus
}
#endif

#endif
#endif // __RTBASE_H__
