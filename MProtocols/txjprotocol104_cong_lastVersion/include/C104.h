#ifndef C104_H
#define C104_H

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include <map>

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
class C104 : public Protocol
{
public:
    C104();
    virtual ~C104();

    void	   Init();
    void	   Uninit();

    void	   OnRead(ST_BYTE* pbuf,ST_INT& readed);
    ST_BOOLEAN OnSend();
    bool	   OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	   IsSupportEngine(ST_INT engineType);
    ST_VOID	   OnConnect(ST_INT port,ST_UINT64 portAddr);
    ST_VOID	   OnDisconnect(ST_INT port,ST_UINT64 portAddr);
protected:

private:
	enum RemoveType
	{
		YXIndex = 0,
		YCIndex = 1,
		CSIndex = 2,
		YKIndex = 3,
		SDIndex = 4,
		YMIndex = 5,
		JTIndex = 6
//		WZIndex = 7,
//		EJIndex = 8
	};
	inline void SendEx (struct iec104_apdu* papdu);
	inline void SendAndCache (struct iec104_apdu* papdu);
	inline void SendNotCache (struct iec104_apdu* papdu);

	bool CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);

	/**
	 * [SendAllYXData  响应总召唤，上送全部遥信]
	 * [SendAllYCData  响应总召唤，上送全部遥测]
	 * @Author    Wedo
	 * @NotesTime 		2016-02-22T16:34:24+0800
	 * @ExpirationTime	2016-08-22T16:34:24+0800
	 */
	void SendAllYXData ();
	void SendAllYCData ();
	void SendAllEqData ();
    void SendALLSDData ();  //上送全部定值
    void SendSomeSDData(ST_BYTE *pbuf);
	/**
	 * [SendStartConfirm  		发送链路启动确认帧]
	 * [SendLinkTestConfirm 	发送链路测试确认帧]
	 * [SendStopConfirm 		发送链路停止确认帧]
	 * [SendTotalCallConfirm 	总召唤确认帧]
	 * [SendEqCallConfirm 		电度总召唤确认帧]
	 * [SendCompareTimeComfirm 	对时确认帧]
	 * [SendNAKComfirm 			发送否定确认返回帧]
	 * @Author    Wedo
	 * @NotesTime 		2016-02-22T17:12:37+0800
	 * @ExpirationTime	2017-02-22T17:12:37+0800
	 */
	void SendStartConfirm    ();
	void SendLinkTestConfirm	();
	void SendStopConfirm		();
	void SendTotalCallConfirm();
	void SendEqCallConfirm   ();
	void SendCompareTimeComfirm (const struct CP56Time2a& ts);
	void SendNAKComfirm (const struct iec104_apdu* apdu, int32_t cause);

	void SendMonitorFrame ();
	void SendTestFrame ();
	/**
	 * [ForwardTellBWYX	 遥信变位上送]
	 * [ForwardTellBWYC	 遥测变位上送]
	 * [ForwardTellSOE	 SOE事件上送]
	 * @Author    Wedo
	 * @NotesTime 2016-03-03T22:46:04+0800
	 * @ExpirationTime 2016-03-03T22:46:04+0800
	 */
	void ForwardTellBWYX	();
	void ForwardTellBWYC	();
	void ForwardTellSOE  (ProtocolTask& task);

	void ForwardTellUnconfirm ();

	/**
	 * [SendTotalCallEnded  发送总召唤结束帧]
	 * [SendEqCallEnded   	发送电度总召唤结束帧]
	 * @Author    Wedo
	 * @NotesTime 2016-03-03T22:46:57+0800
	 * @ExpirationTime 2016-03-03T22:46:57+0800
	 */
	void SendTotalCallEnded  ();
	void SendEqCallEnded 	();


	/**
	 * [TaskHeadlerToLower  从上位机接收到的任务分配处理]
	 * [TaskHeadlerToUpper  从下位机接收到的任务分配处理]
	 * @Author    Wedo
	 * @NotesTime 2016-03-17T15:46:32+0800
	 */
	void TaskHandlerToLower (const struct iec104_apdu* papdu);
	void TaskHandlerToUpper (ProtocolTask& task);

	/**
	 * [TransC_SC_NA_1ToLower  下传单命令到设备]
	 * [TransC_SC_NA_1ToUpper  上送单命令确认到上位机]
	 * @Author    Wedo
	 * @NotesTime 2016-05-27T11:04:45+0800
	 * @ExpirationTime 2016-11-27T11:04:45+0800
	 */
	bool TransC_SC_NA_1ToLower (const struct iec104_apdu* papdu);
	void TransC_SC_NA_1ToUpper (const ProtocolTask& task);

	/**
	 * [TransC_DC_NA_1ToLower  下传双命令到设备]
	 * [TransC_DC_NA_1ToUpper  上送双命令确认到上位机]
	 * @Author    Wedo
	 * @NotesTime 2016-05-27T11:04:45+0800
	 * @ExpirationTime 2016-11-27T11:04:45+0800
	 */
	bool TransC_DC_NA_1ToLower (const struct iec104_apdu* papdu);
	void TransC_DC_NA_1ToUpper (const ProtocolTask& task);

	//lxb
	/**设定值命令, 规一化值
	 * [TransC_SE_NA_1ToLower  下传双命令到设备]
	 * [TransC_SE_NA_1ToUpper  上送双命令确认到上位机]
	 * @Author    Wedo
	 * @NotesTime 2021-12-22T14:22:30+0800
	 * @ExpirationTime 2021-12-22T14:22:30+0800
	 */
	//bool TransC_SE_NA_1ToLower (const struct iec104_apdu* papdu);
	//void TransC_SE_NA_1ToUpper (const ProtocolTask& task);

	/**设定值命令, 标度化值
	 * [TransC_SE_NB_1ToLower  下传双命令到设备]
	 * [TransC_SE_NB_1ToUpper  上送双命令确认到上位机]
	 * @Author    Wedo
	 * @NotesTime 2021-12-22T14:22:30+0800
	 * @ExpirationTime 2021-12-22T14:22:30+0800
	 */
	//bool TransC_SE_NB_1ToLower (const struct iec104_apdu* papdu);
	//void TransC_SE_NB_1ToUpper (const ProtocolTask& task);

	/**设定值命令, 短浮点数
	 * [TransC_SE_NC_1ToLower  下传双命令到设备]
	 * [TransC_SE_NC_1ToUpper  上送双命令确认到上位机]
	 * @Author    Wedo
	 * @NotesTime 2021-12-22T14:22:30+0800
	 * @ExpirationTime 2021-12-22T14:22:30+0800
	 */
	//bool TransC_SE_NC_1ToLower (const struct iec104_apdu* papdu);
	//void TransC_SE_NC_1ToUpper (const ProtocolTask& task);

	int          m_nEQCallStep;
	int          m_nCallStep;
	bool         m_bIsSendSuc;

	uint32_t     m_nSendIdex;
	bool         m_bIsRun;

	FakeTimer    taskexpire;
	ProtocolTask m_curTask;
	int32_t      m_iecTaskType;

    class IEC104Obj*const m_obj;

    class HistoricalValues
	{
	public:
		typedef std::map<int32_t, ST_VARIANT*>::iterator m_iterator;

		HistoricalValues() {}
		~HistoricalValues() { this->clear (); }

		void add (int32_t indexes, ST_VARIANT& sv) {
			ST_VARIANT * temp = this->find (indexes);
			if (temp) {
				memcpy(temp, &sv, sizeof (ST_VARIANT));
			}
			else {
				temp = new ST_VARIANT();
				memcpy(temp, &sv, sizeof (ST_VARIANT));
				save [indexes] = temp;
			}
		}

		ST_VARIANT* find (int32_t indexes) {
			m_iterator temp = save.find (indexes);
			if (temp != save.end())
				return temp->second;
			else
				return NULL;
		}

		void clear () {
			for (m_iterator i = save.begin(); i != save.end(); ++i)
				delete i->second;
			save.clear ();
		}
	private:
		std::map<int32_t, ST_VARIANT*> save;
	} m_hv;

private: // 工程专用，待删除//For Deleting...
	enum TempType {
		SGIndex = 7,
		GJIndex = 8
	};

	void AccidentsHandler (int32_t tableIndex, int32_t addr);


    void                        SendCurFixArea();
    int                         wirteIter;
    float                       writeValue;
	map<ST_DUADDR*,float>       sdMap;
	void                        getSingleWrite(ST_BYTE *pbuf);
	void                        getMultiWrite(ST_BYTE *pbuf);
	void                        transferSingleWriteTask();
	void                        transferMultiWriteTask();
	ST_BYTE                     resendbuf[255];
	void                        recordLog(const char *head,ST_BYTE *pbuf,int len);
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
