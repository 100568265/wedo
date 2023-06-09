#ifndef CIEC101_H
#define CIEC101_H
#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include <queue>
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
typedef unsigned long       DWORD;
using namespace std;
struct COT {
    enum e_type {
	    CYCLIC 			= 1,	// 鍛ㄦ湡銆佸惊鐜?
        BGSCAN 			= 2,	// 鑳屾櫙鎵弿
        SPONTANEOUS 	= 3,	// 鑷彂銆佺獊鍙?
	    INITIALIZE		= 4,	// 鍒濆鍖?
        REQUEST 		= 5,	// 璇锋眰鎴栬璇锋眰
        ACT 			= 6,	// 婵€娲?
        ACTCON 			= 7,	// 婵€娲荤‘璁?
        DEACT 		    = 8,	// 鍋滄婵€娲?
	    DEACTCON		= 9,	// 鍋滄婵€娲荤‘璁?
        ACTTERM 		= 10,	// 婵€娲荤粓姝€佹縺娲荤粨鏉?
	    RETREM         	= 11,	// 杩滄柟鍛戒护寮曡捣鐨勮繑閫佷俊鎭?
	    RETLOC			= 12,	// 褰撳湴鍛戒护寮曡捣鐨勮繑閫佷俊鎭?
	    INROGEN         = 20,	// 鍝嶅簲鎬诲彫鍞?
        INRO1           = 21,   // 鍝嶅簲绗?1缁勫彫鍞?
        INRO2           = 22,   // 鍝嶅簲绗?2缁勫彫鍞?
        INRO3           = 23,   // 鍝嶅簲绗?3缁勫彫鍞?
        INRO4           = 24,   // 鍝嶅簲绗?4缁勫彫鍞?
        INRO5           = 25,   // 鍝嶅簲绗?5缁勫彫鍞?
        INRO6           = 26,   // 鍝嶅簲绗?6缁勫彫鍞?
        INRO7           = 27,   // 鍝嶅簲绗?7缁勫彫鍞?
        INRO8           = 28,   // 鍝嶅簲绗?8缁勫彫鍞?
        INRO9           = 29,   // 鍝嶅簲绗?9缁勫彫鍞?
        INRO10          = 30,   // 鍝嶅簲绗?0缁勫彫鍞?
        INRO11          = 31,   // 鍝嶅簲绗?1缁勫彫鍞?
        INRO12          = 32,   // 鍝嶅簲绗?2缁勫彫鍞?
        INRO13          = 33,   // 鍝嶅簲绗?3缁勫彫鍞?
        INRO14          = 34,   // 鍝嶅簲绗?4缁勫彫鍞?
        INRO15          = 35,   // 鍝嶅簲绗?5缁勫彫鍞?
        INRO16          = 36,   // 鍝嶅簲绗?6缁勫彫鍞?
	    REQCOGEN        = 37,	// 鍝嶅簲鐢靛害閲忔€诲彫鍞?
	    REQCO1         	= 38,	// 鍝嶅簲绗?缁勭數搴﹂噺鍙敜
	    REQCO2         	= 39,	// 鍝嶅簲绗?缁勭數搴﹂噺鍙敜
	    REQCO3         	= 40,	// 鍝嶅簲绗?缁勭數搴﹂噺鍙敜
	    REQCO4         	= 41,	// 鍝嶅簲绗?缁勭數搴﹂噺鍙敜
	    UNKNOWNTYPEID	= 44,	// 鏈煡鐨勭被鍨嬫爣璇?
	    UNKNOWNCAUSE	= 45,	// 鏈煡鐨勪紶閫佸師鍥?
	    UNKNOWNPADDR	= 46,	// 鏈煡鐨凙SDU鍏叡鍦板潃
	    UNKNOWNINFOADDR	= 47	// 鏈煡鐨勪俊鎭璞″湴鍧€
    };
};
class CIEC101 : public Protocol
{
public:
    CIEC101();
    virtual ~CIEC101();

    void	Init();
    void	Uninit();
    ST_BYTE    newday,oldday,sendflag;
    clock_t		Newcurtime,oldcurtime,sendserchtime,CLtime,m_ivRead;
    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    ST_BOOLEAN	OnSend();
    ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
    ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    enum StepTypre
	{
        Undefined = 0,
		RE_ASK_LINK_STATE,
		ASK_LINK_STATE,
		RE_RST_LINK,
		RST_LINK,
		RE_TOTAL_CALL,
		TOTAL_CALL,
		TOTAL_CALL_PH,
		ASK_1ST_DATA,
		ASK_2ND_DATA,
		REMOTE_CTRL_EXEC,
		REMOTE_CTRL_SELECT,
		REMOTE_CTRL_CANCEL,
		PROOFING_TIME
	};
	enum FrameSRState
	{
        INITIAL = 0,
		WAIT1 = 1,
		WAIT2 = 2,
		WAIT3 = 3,
		WAIT_COUNTOUT = 4

    };
protected:

private:
    volatile bool m_isCommStart;
    ProtocolTask m_curTask;
    ST_INT m_curreadIndex;
    ST_INT m_readIndex;
    ST_BOOLEAN  m_b0701;
    std::queue<StepTypre> sendqueue;
    void  SendAck(ST_BYTE byCID);
	//主站请求链路状态
	void  MainAskLinkStatu();
	//主站复位远方链路
	void  MainResetLink();
	//子站请求主站链路状态
	void  SubAskLinkStatu();
	//主站下发总召唤命令
	void  MainAskAllCmd(ST_BYTE fc);
	//时间同步
	void  SendTime(ST_BYTE fc);//对时
	void  CallClass2(ST_BYTE byCID);//召唤2级数据
	void  CallClass1(ST_BYTE byCID);//召唤1级数据
	void  CallPH(ST_BYTE fc);//召唤电度量

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
    void  SendPreYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn);//遥控选择
	void  SendYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn);//遥控执行
	void  SendEndYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn);//遥控结束


    void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);
};

#ifdef _WIN32
	PROTOCOL_API CIEC101* CreateInstace();
#else
	CIEC101* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CIEC101_H
