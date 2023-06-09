#ifndef CSY103_H
#define CSY103_H

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

typedef struct
{
	ST_BYTE FUN:4; //功能码
	ST_BYTE FCV:1; //帧计数位有效
	ST_BYTE FCB:1;//帧计数位
	ST_BYTE PRM:1;//启动报文
	ST_BYTE DIR:1;//方向
}SENDLINKCODE;

class CSY103 : public Protocol
{
    public:
        CSY103();
        virtual ~CSY103();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

        void    ExplainClass2(ST_BYTE* pbuf,ST_INT nlen); //解析2级数据
        void    ExplainCallAll(ST_BYTE* pbuf,ST_INT nlen);//解析总召唤
        void    ExplainEvent(ST_BYTE* pbuf,ST_INT nlen);//解析变位事件
        void    ExplainFixValue(ST_BYTE* pbuf,ST_INT nlen);//解析定值
        void    ExplainSystemFixValue(ST_BYTE* pbuf,ST_INT nlen);//解析定值


        void    LinkReset(void);//复位链路
        void    CallAll(void);//总召唤开始
        void    CallClass1Data(void);//召唤一级数据
        void    CallClass2Data(void);//召唤二级数据
        void    SendTime(void);//对时
        void    Resetdevice(void);//复归装置
        void    ReStart(void);//重新启动
        ST_BOOLEAN    YKSelect(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    YKExecut(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    YKCancel(ST_INT nroute,ST_BYTE byOnOff);
        ST_BOOLEAN    ReadFixValue(ST_INT nfixIndex,ST_INT nfixnum);
        ST_BOOLEAN    WriteFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask);
        ST_BOOLEAN    ReadSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum);
        ST_BOOLEAN    WriteSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask);

//
        ST_BYTE GetCHKSUM(ST_BYTE* pbuf,ST_BYTE bylen);//校验
        bool    m_bFCB;//帧计数位
        bool    m_bresetlink;//复位链路
        bool    m_bcallall;//正在总召唤数据
        bool    m_bcallclass1;//总召唤一级数据
        bool    m_bcallclass2;//总召唤二级数据
        bool    m_blink;//链路状况

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT  m_nroute;
        ST_BYTE m_byOnOff;
        struct timeval  m_Timeout;
        time_t m_tlast;
    protected:
    private:
};

#ifdef _WIN32
	PROTOCOL_API CSY103* CreateInstace();
#else
	CSY103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CSY103_H
