#ifndef CHEIGHLIGHT_H
#define CHEIGHLIGHT_H

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
class CHeighLight : public Protocol
{
    public:
        CHeighLight();
        virtual ~CHeighLight();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_BYTE m_Statu[16];
    	ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;

        void  SendReadCmd(ST_BYTE code, ST_UINT readAddr,ST_UINT count);
        void FillWORD(ST_BYTE* buf,ST_UINT v);
};

#ifdef _WIN32
	PROTOCOL_API CHeighLight* CreateInstace();
#else
	CHeighLight* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CHEIGHLIGHT_H
