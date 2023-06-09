#ifndef CKB370_H
#define CKB370_H

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

class CKb370 : public Protocol
{
    public:
        CKb370();
        virtual ~CKb370();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_INT m_curreadIndex;
        ST_INT m_nIndex;
        void GetYC(void);
        void GetYX(void);
};

#ifdef _WIN32
	PROTOCOL_API CKb370* CreateInstace();
#else
	CKb370* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CKB370_H
