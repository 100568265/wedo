#ifndef _CIEC101_H_
#define _CIEC101_H_

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"

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
using namespace std;
class CIEC101 : public Protocol
{
public:
    CIEC101();
    virtual ~CIEC101();

    void	Init();
    void	Uninit();

    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
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
	inline void SendEx (ST_BYTE* data);

	bool CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);

	void AllYXDataPacking ();
template<int TYPE>
	void AllYCDataPacking ();
	void AllEqDataPacking ();

	void SendFixedFrame (const CtrlField& cfref);

	ST_BOOLEAN m_datavalue[1024];
	float m_fdatavalue[1024];

	CtrlField GetCurCtrlField (FC::CLIENT func);

	/**
	 * [SendEqCallConfirm 		电度总召唤确认帧]
	 * [SendCompareTimeComfirm 	对时确认帧]
	 * @Author    Wedo
	 * @NotesTime 		2016-02-22T17:12:37+0800
	 * @ExpirationTime	2017-02-22T17:12:37+0800
	 */
	void KeepTotalCallConfirm();
	void SendEqCallConfirm   (int32_t qccfrz);
	void KeepCompareTimeComfirm (const struct CP56Time2a& ts);
	void KeepNAKComfirm (const ST_BYTE* data, int32_t cause);

	/**
	 * [CheckBWYX	 遥信变位上送]
	 * [CheckBWYC	 遥测变位上送]
	 * [ForwardTellSOE	 SOE事件上送]
	 * @Author    Wedo
	 * @NotesTime 2016-03-03T22:46:04+0800
	 * @ExpirationTime 2016-03-03T22:46:04+0800
	 */
	void CheckBWYX ();
template<int TYPE>
	void CheckBWYC ();
	void ForwardTellSOE  (ProtocolTask& task);

	/**
	 * [SendTotalCallEnded  发送总召唤结束帧]
	 * [SendEqCallEnded   	发送电度总召唤结束帧]
	 * @Author    Wedo
	 * @NotesTime 2016-03-03T22:46:57+0800
	 * @ExpirationTime 2016-03-03T22:46:57+0800
	 */
	void KeepTotalCallEnded ();
	void KeepEqCallEnded 	();
	void KeepInitEnded      ();

	void SendTier1Data ();
	void SendTier2Data ();
	void CleanQueue    ();

	void TotalCallHelper (const uint8_t* data);
	void EqCallHelper    (const uint8_t* data);
	/**
	 * [TaskHeadlerToLower  从上位机接收到的任务分配处理]
	 * [TaskHeadlerToUpper  从下位机接收到的任务分配处理]
	 * @Author    Wedo
	 * @NotesTime 2016-03-17T15:46:32+0800
	 */
	void TaskHandlerToLower (const ST_BYTE* data);
	void TaskHandlerToUpper (ProtocolTask& task);

	/**
	 * [TransC_SC_NA_1ToLower  下传单命令到设备]
	 * [TransC_SC_NA_1ToUpper  上送单命令确认到上位机]
	 */
	void TransC_SC_NA_1ToLower (const ST_BYTE* data);
	void TransC_SC_NA_1ToUpper (const ProtocolTask& task);

	/**
	 * [TransC_DC_NA_1ToLower  下传双命令到设备]
	 * [TransC_DC_NA_1ToUpper  上送双命令确认到上位机]
	 */
	void TransC_DC_NA_1ToLower (const ST_BYTE* data);
	void TransC_DC_NA_1ToUpper (const ProtocolTask& task);

	bool m_bIsSendSuc;

    FakeTimer taskexpire;
    ProtocolTask m_curTask;
	int32_t m_iecTaskType;

    class IEC101Obj*const m_obj;
    clock_t		Newcurtime,sendserchtime;
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

	uint8_t temindex;
	void AccidentsHandler (int32_t tableIndex, int32_t addr);
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
