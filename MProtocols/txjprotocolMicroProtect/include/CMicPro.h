#ifndef CMICPRO_H
#define CMICPRO_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include <deque>
//#include <unordered_map>
#include <map>
#include <array>
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

#define END_CODE 0x03
enum SENDTYPE
{
    QUERYDATA = 0x48,
    C12SOE = 0x73,
    ACK = 0x06,
    NAK = 0x05,
    YKSELECT = 0x70,
    YKEXECUT = 0x71
};

struct TypeCode{
    enum code{
        RACK = 0x06,
        SOE = 0x40,
        EVENT = 0x41,
        YCYXDATA = 0x4D,
        EQDATA = 0x55,     //电能数据
        YKSELRETURN = 0x70,
        YKEXESUCCESS = 0x15
    };
};

struct DataAreaNum{
    enum Num{
        YCAREA = 0x00,
        YXAREA,
        SOEAREA,
        EVENTAREA,
        EQAREA
    };
};
//struct Data
class CMicPro: public Protocol
{
    public:
        CMicPro();
        virtual ~CMicPro();

        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);



    protected:

    private:

        deque<SENDTYPE>             m_sendqueue;
        ProtocolTask                m_curTask;
        bool                        m_ykargv;
        bool                        m_isYK;

        void    SendYkSelect(bool argv);
        void    SendYkExecut(bool argv);

        void    SendEasyOrder(SENDTYPE sendtype);

        void    AnalyzerEq(ST_BYTE *pbuf);
        void    AnalyzerYCYX(ST_BYTE *pbuf);
        void    AnalyzerSOE(ST_BYTE *pbuf);
        void    AnalyzerEvent(ST_BYTE *pbuf);


        int     getDataAreaItem(int fun,int inf,ST_DataAreaItem& itemref);
        void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);

//        unordered_map<int ,vector<ST_BYTE> *> m_datamap;
        map<int ,vector<ST_BYTE> > m_datamap;
        void    parseYC();
        void    parseYX();
        void     ClearData();

};

#ifdef _WIN32
	PROTOCOL_API CMicPro* CreateInstace();
#else
	CMicPro* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CMICPRO_H
