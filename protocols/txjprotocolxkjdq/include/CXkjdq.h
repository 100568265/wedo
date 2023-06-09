#ifndef CXKJDQ_H
#define CXKJDQ_H
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
class CXkjdq : public Protocol
{
    public:
        CXkjdq();
        virtual ~CXkjdq();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT  m_nroute;   //遥控ID
        ST_BYTE m_byOnOff;  //选择闭合
        ST_BOOLEAN    YKExecut(ST_INT nroute,ST_BYTE byOnOff);

};

#endif // CXKJDQ_H

#ifdef _WIN32
	PROTOCOL_API CXkjdq* CreateInstace();
#else
	CXkjdq* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
