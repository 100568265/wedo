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
typedef uint16_t WORD;
typedef uint32_t DWORD;

class CInner : public Protocol
{
    public:
        enum {
		RECV_VARIABLE  = 0x01,	// 接收到一般数据
		RECV_SOE_TASK  = 0x02,  // 接收到SOE
		RECV_DEV_STATE = 0x03,  // 接收到设备状态

		REMOTE_CTRL_ASK = 0x10, // 遥控任务下发
		REMOTE_CTRL_CON = 0x80, // 遥控任务返回

		USER_TASK_ASK = 0x20,   // 用户自定义任务下发
		USER_TASK_CON = 0x40,   // 用户自定义任务返回

	 // TOTAL_CALL_ASK = 0xFF
		TOTAL_CALL_CON = 0xFF,  // 总召唤下发和返回

		SHAKE_ASK = 0x07,       // 握手下发
		SHAKE_CON = 0x0B,       // 握手确认

	    HEARTBEAT_ASK = 0x43,   // 心跳下发
		HEARTBEAT_CON = 0x83,   // 心跳确认

		SLAVE_REBOOT_MSG = 0x2B // 子站重启信息
	};

        CInner();
        virtual ~CInner();
        void	   Init();
        void	   Uninit();

        void	   OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN OnSend();
        ST_BOOLEAN	   OnProcess(ST_BYTE* pbuf,int len);
        int	    GetProtocolType();
        ST_BOOLEAN	   IsSupportEngine(ST_INT engineType);
        void  SendPreYK(WORD wAddr,ST_BOOLEAN bYkOn);//遥控选择
        void  SendYK(WORD wAddr,ST_BOOLEAN bYkOn);//遥控执行


    protected:
        ProtocolTask m_curTask;
        ST_BOOLEAN m_bTask;
        ST_BOOLEAN           m_bStart;

        DWORD  m_nowtickcount;
        DWORD  m_ndevicestatu;



    private:

};
#ifdef _WIN32
	PROTOCOL_API CInner* CreateInstace();
#else
	CInner* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // C104_H
