#ifndef CALLGO_H
#define CALLGO_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include "Channel.h"


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

class CAllGo: public Protocol
{
public:
    CAllGo();
    virtual ~CAllGo();

    void	Init();
    void	Uninit();
    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    ST_BOOLEAN	OnSend();
    ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
    ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    void    ReadData(int idex);
    void    ReadStatus();
    void    processBuf(ST_BYTE* pbuf);
    void    SendYK(ST_UINT writeAddr);  //红外遥控YK
    void    SendKG(ST_UINT writeAddr,ST_BOOLEAN bIsOn);   //智能开关YK

    void    sendCLKZ(ST_BOOLEAN bIsOn,ST_UINT writeAddr);  //智能窗帘控制
    void    sendCLSTOP(ST_BOOLEAN bIsOn,ST_UINT writeAddr);  //智能窗帘停止命令

protected:
private:

    bool m_isInit;
    int  m_devSign;       //设备模板标志 区分什么设备
    int  m_rIndex;

    float humidity;     //湿度
    float hum_Inter;    //整数
    float hum_decimal;  //小数

    float temperature;  //温度
    float temp_Inter;    //整数
    float temp_decimal;  //小数

    List<ProtocolBase *>	t_Protocols ;
    time_t		Newcurtime,oldcurtime;

    ProtocolTask m_curTask;

    //ST_BYTE  BCD2Hex(ST_BYTE byte);

    int devstauts;

};

#ifdef _WIN32
PROTOCOL_API CAllGo* CreateInstace();
#else
CAllGo* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CALLGO_H
