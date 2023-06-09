#ifndef C103_H
#define C103_H

#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà
#include "datetime.h"
#include "time.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif
#ifdef __cplusplus

extern "C"
{
#endif
//using namespace std;

// 信息元
#pragma pack(push,1)
struct ASDU21_Unit
{
    ST_UINT16   gin;
    ST_BYTE     kod;
};

struct ASDU21
{
  ST_BYTE   cot;
  ST_BYTE   fun;
  ST_BYTE   inf;
  ST_BYTE   rii;
  ST_BYTE   nog;
  ST_BYTE*  unitbuf;
  ST_BYTE   unitbuf_size;
  // ASDU21_Unit unitbuf[nog];  // 应该不行
};
//#pragma pack(pop)

class C103 : public Protocol
{
    public:
        C103();
        virtual ~C103();

        void	Init();
        void	Uninit();

        void	OnRead(ST_BYTE * pbuf, ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);



    protected:
    private:
        void    YKSelect  (ProtocolTask & task);
        void    YKExecut  (ProtocolTask & task);
        void    YKCancel  (ProtocolTask & task);
        void    Send_ADSU21 (ASDU21 & buf);
        void    AskGroupData (ST_BYTE GroupNo);         // 读一组配置全部条目的实际值
        void    CallAll_ASDU21();
        void    CallAll_ASDU7();
        void    Clocksync_ASDU6();
        void    AnalysisASDU10(ST_BYTE *buf,ST_INT & cnt);
        void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);
        void    CreateUDP(void* px);
        void    InitUDPClient();
        int     fd;
        int     ret;

        ST_BOOLEAN  m_nStart;
        ST_BYTE     _sendstate;
        ST_BOOLEAN  _callalled;
        clock_t     _lastcalltime;
        ProtocolTask m_curTask;
        ST_BOOLEAN  m_bTask;

        ST_BOOLEAN  _acked;
        clock_t     _lastreadtime;
};
#ifdef _WIN32
	PROTOCOL_API C103* CreateInstace();
#else
	C103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C103_H
