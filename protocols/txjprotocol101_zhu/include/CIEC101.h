#ifndef _CIEC101_H_
#define _CIEC101_H_

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include <queue>
#include "FakeTimer.h"
#include "iec60870_5_101_types.h"
#include <map>

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
//extern "C"
//{
#endif
#define wDebug(_ptr) if (debugFlag) wedoDebug(this).noquote()
using namespace std;
class CIEC101 : public Protocol
{
public:
    CIEC101();
    virtual ~CIEC101();

    void	Init();
    void	Uninit();
    ST_BYTE    newday,oldday,sendflag;
    clock_t		Newcurtime,oldcurtime,sendserchtime,CLtime;
    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
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
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CIEC101* CreateInstace();
#else
	CIEC101* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // _CIEC101_H_
