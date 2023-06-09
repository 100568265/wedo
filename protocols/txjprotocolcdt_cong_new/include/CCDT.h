#ifndef CCDT_H
#define CCDT_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class CCDT : public Protocol
{
    enum RemoveType
    {
        YXIndex = 0,//10,
//      BHIndex = 1,
        YCIndex = 1,//11,
//      CSIndex = 3,
        YKIndex = 15,
//      SDIndex = 5,
        YMIndex = 12,
//      WZIndex = 7,
//      EJIndex = 8
    };
    enum TempType {
        SGIndex = 13,
        GJIndex = 14,
    };
public:
        CCDT();
        virtual ~CCDT();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:

    private:
        bool CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);

        void    SendAllYX();
        void    SendAllYC();
        void    SendAllYM();
        void    Readlimit();
        void    ReadGJlimit();
        void    SendSoe(ProtocolTask& task);
        void    SendGJSoe(ST_BYTE SendSoeNo,ST_BOOLEAN BVal);
        ProtocolTask  m_task;
        ST_INT m_nSendOrder;
        ST_INT m_Timeout;
        ST_BOOLEAN Bval;
        ST_BOOLEAN GjSendstate,SGSendstate;
};

#ifdef _WIN32
	PROTOCOL_API CCDT* CreateInstace();
#else
	CCDT* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CCDT_H
