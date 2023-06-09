#ifndef PMC_H
#define PMC_H

#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà


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

class CPMC : public Protocol
{
    public:
        CPMC();
        virtual ~CPMC();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    private:
        struct RTValueType
        {
            enum TYPE
            {
                ALLDATA = 0xAA,		// 一帧读取所有实时数据寄存器(保护遥信遥测 (40000 ~ 40018)
                SYSTEMDATA = 0XBB,	// 一帧读取系统参数寄存器(51000 ~ 51014)
                PTDATA = 0xCC,		// 一帧读取保护参数寄存器(51400 ~ 51440)
                YXDATA = 0XDD		// 一帧读取遥信数据 (40000 ~ 40007)
            };
        };

        ST_BOOLEAN	m_portbreak;
        ST_BYTE     m_curReadIndex;		// 当前实时值读取的情况
        ST_UINT32   soeptr;				// SOE总指针


        uint16_t GetCRC16 (const uint8_t *pdata, int nsize);
        void	AskSoe(ST_UINT32 ptr);
        void	AskRTValue(ST_INT type);
        void	SetParam(ST_BYTE typ, ST_UINT16 addr, ST_BYTE *datas);

        ST_UINT32	GetDWORD(ST_BYTE *buf);
};

#ifdef _WIN32
	PROTOCOL_API CPMC* CreateInstace();
#else
	CPMC* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // PMC_H
