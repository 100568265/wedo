#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "datatype.h"
#include "ProtocolBase.h"
#include "CmnRtInterface.h"

class Channel;
class Device;

#ifdef _WIN32
class COMMUNICATIONDLL_API Protocol : public ProtocolBase
#else
class Protocol : public ProtocolBase
#endif
{
public:
    Protocol();
    virtual ~Protocol();
    ST_VOID UpdateValue(ST_INT addrNo,ST_BYTE    value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_UINT32  value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_UINT64  value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_CHAR    value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_INT16   value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_INT     value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_FLOAT   value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_DOUBLE  value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_CHAR*   value,ST_INT channelId=-1,ST_INT deviceId=-1);
    ST_VOID UpdateValue(ST_INT addrNo,ST_DECIMAL value,ST_INT channelId=-1,ST_INT deviceId=-1);
//    List<ST_Variable> *GetVariables(ST_CHAR *fullName,ST_INT addr);
	ST_INT GetVariableValue(ST_CHAR *fullName,ST_VARIANT &var);
	ST_INT GetVariableValue(ST_DUADDR addr,ST_VARIANT &var);
};
typedef Protocol CProtocol;

#ifdef PROTOCOL_EXPORTS_TYPE
    #define _WIN_PROTOCOL_PLUGIN_EXPORT_ __declspec(dllexport)
#else
    #define _WIN_PROTOCOL_PLUGIN_EXPORT_ __declspec(dllimport)
#endif

#ifdef _WIN32
    #define _PROTOCOL_PLUGIN_EXPORT_ _WIN_PROTOCOL_PLUGIN_EXPORT_
#else
    #define _PROTOCOL_PLUGIN_EXPORT_
#endif

#define _PROTOCOL_INTERFACE_DECLARATION_(class_name)                  \
    extern "C"                                                        \
    {                                                                 \
        _PROTOCOL_PLUGIN_EXPORT_ class Protocol* CreateInstace();     \
    }


#define _PROTOCOL_INTERFACE_IMPLEMENT_(class_name)                    \
    class Protocol* CreateInstace() { return new class_name(); }

#endif // PROTOCOL_H
