#ifndef C103_H
#define C103_H
#include "Protocol.h"                                         //规约父类
#include "DataCache.h"                                    //数据缓冲类
#include "Device.h"                                             //通讯设备类
#include "Devices.h"                                           // 通讯设备管理类
#include "sysinifile.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

//using namespace std;

class C103:public Protocol
{
                public:
                                C103();
                                virtual ~C103();
                                void	Init();
                                void	Uninit();
                                void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
                                ST_BOOLEAN	OnSend();
                                ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
                                ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
                                clock_t		Newcurtime,oldcurtime,sendserchtime,CLtime;
                                void	                SendASDU21();                                   //通用分类命令--召唤遥测量、定值、各种配置列表
                                void	                SendASDU6();                                      //时间同步 对时
                                void                       ASDU2a(ST_BYTE* Rebuf);
                                void                       ASDU1(ST_BYTE* Rebuf);               //上送压板及告警等开关量状态
                                void	                 ASDU10(ST_BYTE* Rebuf);             //通用分类数据-->下传定值、切换定值区、遥控（确认和执行）
                                void                       ASDU2(ST_BYTE* Rebuf);               //上送保护动作信息
                                ProtocolTask      m_curTask;
                                ST_BYTE    newday,oldday,sendflag;
                private:
                                void	                SendSetframe(ST_BYTE code);
                                ST_BYTE              SendState;
                                ST_BYTE               FCB;
                                ST_BYTE                BreakCallState;
                                void    EXpainYc(ST_BYTE* Rebuf);
                                void    EXpainYx(ST_BYTE* Rebuf);
                                void    EXpainBwYx(ST_BYTE* Rebuf);
                                void    EXpainSOE(ST_BYTE* Rebuf);
                                void    SendAllSearch();
                                void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API C103* CreateInstace();
#else
	C103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C103_H
