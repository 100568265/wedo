#ifndef C104_H
#define C104_H

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"

#include "FakeTimer.h"
#include <map>

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

#define BIGLEN  (255)

struct PClientLink{
	ST_INT Len;
	ST_BYTE FrameNo[2];
	ST_BOOLEAN TESTFR;          //网络是否连通
	ST_BOOLEAN STARTDT;         //是否开始发送数据
	ST_BOOLEAN Ploy;            //有没有激活连接
	ST_BOOLEAN First;           //第一次激活
};
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
#define M_SP_NA_1 (1)  //单点信息
#define M_SP_TA_1 (2)  //带时标的单点信息
#define M_DP_NA_1 (3)  //双点信息
#define M_DP_TA_1 (4)  //带时标的双点信息
#define M_SP_TB_1 (30) //带CP56Time2a时标的单点信息
#define M_DP_TB_1 (31) //带CP56Time2a时标的双点信息
#define M_ME_NA_1 (9)  //测量值，规一化值
#define M_ME_NB_1 (11) //测量值，标度化值
#define M_ME_NC_1 (13) //测量值，短浮点数
#define M_ME_ND_1 (21) //测量值，不带品质描述词的规一化值
#define C_SC_NA_1 (45) //单点遥控
#define C_DC_NA_1 (46) //双点遥控
#define M_IT_NA_1 (15) //电能脉冲量
#define M_IT_TB_1  37

struct CP56Time2a
{
	unsigned short Millisecond;
	ST_BYTE Minute;
	ST_BYTE Hour;
	ST_BYTE Day;
	ST_BYTE Month;
	short Year;
};

class C104 : public Protocol
{
    public:
        C104();
        virtual ~C104();
        void	   Init();
        void	   Uninit();

        void	   OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN OnSend();
        ST_BOOLEAN	   OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	   IsSupportEngine(ST_INT engineType);

        void SureFrame();
        void STARTDT();
        void ClearClientLink();
        void CallAll();

        void Explain_M_SP_NA_1(ST_BYTE* pbuf,ST_INT len);  //单点信息
        void Explain_M_SP_TA_1(ST_BYTE* pbuf,ST_INT len);  //带时标的单点信息
        void Explain_M_DP_NA_1(ST_BYTE* pbuf,ST_INT len);  //双点信息
        void Explain_M_DP_TA_1(ST_BYTE* pbuf,ST_INT len);  //带时标的双点信息
        void Explain_M_SP_TB_1(ST_BYTE* pbuf,ST_INT len); //带CP56Time2a时标的单点信息
        void Explain_M_DP_TB_1(ST_BYTE* pbuf,ST_INT len); //带CP56Time2a时标的双点信息
        void Explain_M_ME_NA_1(ST_BYTE* pbuf,ST_INT len);  //测量值，规一化值
        void Explain_M_ME_NB_1(ST_BYTE* pbuf,ST_INT len); //测量值，标度化值
        void Explain_M_ME_NC_1(ST_BYTE* pbuf,ST_INT len); //测量值，短浮点数
        void Explain_M_ME_ND_1(ST_BYTE* pbuf,ST_INT len); //测量值，不带品质描述词的规一化值
        void Explain_M_IT_NA_1(ST_BYTE* pbuf,ST_INT len); //电能脉冲量
        void Explain_M_IT_TB_1(ST_BYTE* pbuf,ST_INT len);

        void  SendPreYK(WORD wAddr,ST_BOOLEAN bYkOn);//遥控选择
        void  SendYK(WORD wAddr,ST_BOOLEAN bYkOn);//遥控执行
        void  CallPH(void);//召唤电度量
        void  SendTime();
        void  TransferEx(ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);
    protected:
    private:
        volatile unsigned short m_VS;
        volatile unsigned short m_VR;
        unsigned long m_readlasttime;
        unsigned long m_timeoutReceive;
        unsigned long m_timecallall;
        PClientLink m_pcl;

        ST_BOOLEAN m_bTask;
        ProtocolTask m_curTask;
        ST_INT m_curreadIndex;
        ST_INT m_readIndex;

        ST_INT  m_nStart;
        ST_INT nSendIndex;
        ST_BOOLEAN  m_b0701;
        ST_BOOLEAN  m_bCallPH;
        ST_INT   m_PHsum;

        ST_INT nrecnem;
        ST_INT indexN;
};
#ifdef _WIN32
	PROTOCOL_API C104* CreateInstace();
#else
	C104* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C104_H
