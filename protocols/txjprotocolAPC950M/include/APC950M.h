#ifndef APC950M_H
#define APC950M_H

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


class APC950M: public Protocol
{
    enum FUNCSTEP{
        FBATCH = 0,
        FRDNODE = 1,
        FSTATUS = 2,
        FCMD = 3,
        FRST = 4
    };
    enum NETSTATUS{
        BUSY_STATUS = 0,
        FREE_STATUS = 1
    };

    public:
        APC950M();
        virtual ~APC950M();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

        void  analyzeBATCH(ST_BYTE* pbuf,ST_INT len);
        void  analyzeCMD(ST_BYTE* pbuf,ST_INT len);

        void  SendBATCH();
        void  SendNDNODE();
        void  SendNDNODE(int ID);
        void  SendCMD(ST_UINT writeAddr);
        void  SendSTATUS();
        void  SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn);

        ST_BYTE switchValue(ST_BYTE bvalue);
        bool  m_ykcmd = false;  //用于遥控后，空闲后执行CMD单抄
        bool  m_task;
        ProtocolTask m_curTask;

        bool  m_isBatch = false;
        int   m_sendCount =0;
        int   m_readidex = 0;
        int   step = FBATCH;
        bool  m_net_status = true;
        ST_UINT ykAddr = 0;

        time_t		Newcurtime,oldcurtime;

    protected:
    private:
};

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	APC950M* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // APC950M_H
