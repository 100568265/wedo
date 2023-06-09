#include "CIEC101.h"
#include "sysmutex.h"
#include "iec60870_5_101_types.h"
#include "iec60870_5_101_obj.h"
#include "datetime.h"
#include "Debug.h"
#include "FakeTimer.h"
#include <cmath>
#include <queue>

volatile time_t FakeTimer::curr_sec;

#define logWarn		SysLogger::GetInstance()->LogWarn
#define sDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote

// extern NodeTree *g_pTree;

class SQueue
{
	typedef std::queue<uint8_t*> Type;
public:
	SQueue() {}
	virtual ~SQueue() {}

	inline bool Empty () {
		return __queue.empty ();
	}

	inline int32_t Size () {
		return __queue.size();
	}

	void Push (uint8_t* data, int len) {
		if (__queue.size () > 4096) return;

		uint8_t* temp = new uint8_t[len];
		memcpy(temp, data, len);
		Locker locker(&(this->__mutex));
		__queue.push (temp);
	}

	inline uint8_t* Front () {
		return __queue.front();
	}

	void Pop () {
		Locker locker(&(this->__mutex));
		__queue.pop ();
	}
private:
	Type  __queue;
	Mutex __mutex;
};

class IEC101Obj
{
 public:
	IEC101Obj() : is1stStart(true), YXiter(0), YCiter(0) {}
	~IEC101Obj() {}

	bool    is1stStart;
	int32_t YXiter;
	int32_t YCiter;

	SQueue squeue1st;
	SQueue squeue2st;
	Option option;

	CtrlField cfsave;

	std::vector<uint8_t> framesave;
}; // ! class IEC101Obj

CIEC101* CreateInstace()
{
	return new CIEC101 ();
}

CIEC101::CIEC101() : taskexpire (10), m_obj(new IEC101Obj())
{
	//ctor
}

CIEC101::~CIEC101()
{
	//dtor
	delete m_obj;
}
volatile int changeNum = 1;
void CIEC101::Init ()
{
	m_iecTaskType = TYP::Undefined;

	int32_t channeliv = this->m_pChannel->GetChannelInfo()->ChannelInterval;
	m_obj->option.outime = max (channeliv, 2000);

//	if (!this->GetDevice())
 //   	return;
 /*
	m_obj->option.laddr = this->GetDevice()->GetDeviceInfo()->Address;
	m_obj->option.laddr1 = atoi(this->GetDevice()->GetDeviceInfo()->Addressex);
	m_obj->option.paddr = this->GetDevice()->GetDeviceInfo()->Address;
	m_obj->option.paddr1 = atoi(this->GetDevice()->GetDeviceInfo()->Addressex);
	*/
	m_obj->option.laddr = 0x47;
	m_obj->option.laddr1 = 0x20;
	m_obj->option.paddr = 0x47;
	m_obj->option.paddr1 = 0x20;
	// this->GetDevice()->GetDeviceInfo()->AddressEx;
    Newcurtime=clock();
	sendserchtime=clock();
}

void CIEC101::Uninit()
{
}

void CIEC101::OnRead (ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	do {
		if (! this->IsOpened())  {
			break;
		}
		if(! this->GetCurPort()) {
			break;
		}
		int32_t	len = 0;
		len = this->GetCurPort()->PickBytes(pbuf, 6, 5000);
		if (len < 1) {
			break;
		}
		int32_t star = 0;
		for(; star < len; ++star)
		{
			if(pbuf[star] == 0x68 || pbuf[star] == 0x10)
				break;
		}
		if(star == len)
		{
			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName
				     << "] All Data is messy code to clear,\n SurPacket :"
					 <<  CDebug::bytesToHexString (pbuf, len);

			this->GetCurPort()->Clear();
			break;
		}
		if(star > 0)
		{
			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName
					 << "] Part Data is messy code to clear,\n SurPacket :"
					 <<  CDebug::bytesToHexString (pbuf, len);
			this->GetCurPort()->ReadBytes(pbuf, star);
		}

		len = this->GetCurPort()->PickBytes(pbuf, 6, 3000);
		if (pbuf[0] != 0x68 && pbuf[0] != 0x10) {
			break;
		}
		if(pbuf[0] == 0x10)
		{
            int32_t ndatalen = Parser::GetSize (pbuf);
            len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
            m_pLogger->LogDebug("8.%d,%d",ndatalen,len);
          //  string sss;
          //  sss.Format("8.%d,%d",ndatalen,len);
          //   ShowMessage (sss);
            readed = len;
            break;
		}
		else
		{
            int32_t ndatalen = Parser::GetSize (pbuf);
            len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
            ShowMessage ("88.");
            if(len != ndatalen) {
                sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName
                         << "] Data Length is under to clear,\n SurPacket :"
                         <<  CDebug::bytesToHexString (pbuf, len);
                this->GetCurPort()->Clear();
                ShowMessage ("899.");
                break;
            }
            if (Parser::CheckError(pbuf)) {
                sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName
                         << "] Check Sum Error,\n SurPacket :"
                         <<  CDebug::bytesToHexString (pbuf, len);
                this->GetCurPort()->Clear();
                ShowMessage ("800.");
                break;
            }
            ShowMessage ("8.");
            readed = len;
       }

	} while (0);
}

bool CIEC101::OnSend ()
{
    Newcurtime=clock();
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"SOE"))
		{
			ForwardTellSOE (m_curTask);
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
//			TaskHandlerToUpper (m_curTask);
			return true;
		}
		return false;
	}

	FakeTimer::timing ();

	if (! m_bIsSendSuc) {
	}

	if (taskexpire.isTimeout()) {
		taskexpire.stop ();
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "][task time out.]";
		m_iecTaskType = TYP::Undefined;
		ShowMessage("226");
	}

	CheckBWYX ();
	Thread::SLEEP(100);
	CheckBWYC<TYP::M_ME_NC_1> ();
	temindex = ++temindex % 2;
	AccidentsHandler (SGIndex + temindex, temindex);

	Thread::SLEEP (100);
	return true;
}

inline bool operator== (const CtrlField& lhs, const CtrlField& rhs)
{
	return *((uint8_t*)&lhs) == *((uint8_t*)&rhs);
}

bool CIEC101::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
    Parser parser(pbuf, &(m_obj->option));
	if (parser.isError())
	{
		return false;
}
	CtrlField cfrecv = parser.ctrlField();

//	if (! cfrecv.prm)	// 只接受主站报文
//	{
//		return true;

//	}				// 重发机制
//	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "][Receive frame.]";

	if (cfrecv.fd != 0 && cfrecv == m_obj->cfsave)
	{
		if (! m_obj->framesave.empty())
			this->SendEx ((uint8_t*)(&(m_obj->framesave[0])));
		return true;
	}
	else {
		m_obj->cfsave = cfrecv;
	}
	switch (cfrecv.fc) {
		case FC::RST_LINK:       {	// 复位远方链路
            if(cfrecv.prm)
            {
                CleanQueue ();
                if (m_obj->is1stStart) {
                    m_obj->is1stStart = false;
                    KeepInitEnded ();
                }
                SendFixedFrame (GetCurCtrlField (FC::CONFIRM));
                CtrlField cf;
                cf.dir = 1;
                cf.prm = 1;
                cf.fa  = 0;
                cf.fd  = 0;
                cf.fc  = 9;
                SendFixedFrame(cf);
            }
            else
            {

            }
		} break;
		case FC::RST_USER_PRO:   {	// 复位用户进程
		} break;
		case FC::TEST_LINK:      {
                CtrlField cf;
                cf.dir = 0;
                cf.prm = 0;
                cf.fa  = 0;
                cf.fd  = 0;
                cf.fc  = FC::CONFIRM;
                SendFixedFrame (cf);
		} break;
		case FC::SEND_DATA:      {
		} break;
		case FC::BROADCAST_DATA: {
		} break;
		case FC::VISIT_ASK:      {
		} break;
		case FC::ASK_LINK_STATE: {	// 请求链路状态
            if(cfrecv.prm)
            {
                CtrlField cf;
                cf.dir = 1;
                cf.prm = 0;
                cf.fa  = 0;
                cf.fd  = 0;
                cf.fc  = FC::LINK_PERFECT;
                SendFixedFrame (cf);
            }
		} break;
		case FC::ASK_TIER1_DATA: {	// 请求1级数据
		ShowMessage ("15.");
			SendTier1Data ();
		} break;
		case FC::ASK_TIER2_DATA: {	// 请求2级数据
		if(cfrecv.prm)
		{
            ShowMessage ("16.");
			SendTier2Data ();
        }
        else{
                CtrlField cf;
                cf.dir = 1;
                cf.prm = 1;
                cf.fa  = 0;
                cf.fd  = 0;
                cf.fc  = 0;
                SendFixedFrame (cf);
        }
		} break;
	}

	switch (parser.typeId()) {
		case TYP::C_SC_NA_1:   	// 单点命令
		case TYP::C_DC_NA_1:   	// 双点命令
        case TYP::C_RC_NA_1:   	//
		case TYP::C_SE_NA_1:   	//
		case TYP::C_SE_NB_1:   	//
		case TYP::C_SE_NC_1: {  //
			TaskHandlerToLower (pbuf);
			ShowMessage ("1771.");

        } break;

		case TYP::C_IC_NA_1: {	// 总召唤命令
		ShowMessage ("1772.");
			TotalCallHelper    (pbuf);
		} break;

		case TYP::C_CI_NA_1: {	// 电度总召唤
		ShowMessage ("1773.");
			EqCallHelper       (pbuf);
		} break;

		case TYP::C_CS_NA_1: {  // 时钟同步
		ShowMessage ("1774.");
			APDU<TYP::C_CS_NA_1>* papdu = (APDU<TYP::C_CS_NA_1>*) pbuf;
			KeepCompareTimeComfirm (papdu->nsq[0].obj.ts);
			SendFixedFrame (GetCurCtrlField (FC::CONFIRM));
		} break;

		case TYP::Undefined: {
		ShowMessage ("17.");
		} break;
		default : {
		ShowMessage ("18.");
			KeepNAKComfirm (pbuf, Cause::UNKNOWNTYPEID);
			SendFixedFrame (GetCurCtrlField (FC::CONFIRM));
		} break;
	}
	return true;
}

bool CIEC101::IsSupportEngine (ST_INT engineType)
{
	return 0 == engineType;
}

void CIEC101::SendEx (uint8_t* data)
{
	int32_t len   = Parser::GetSize(data);
	if (!len) return;
	data[len - 2] = Parser::GetCheckSum(data);
	data[len - 1] = APCI::STOPCHR;
	m_bIsSendSuc  = this->Send(data, len);
}

bool CIEC101::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
	if (!this->GetDevice())
		return false;
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables) {
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] No transfer tables.";
		return false;
	}
	if (trantables->GetCount() <= 0) {
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Transfer table count is 0.";
		return false;
	}
	int iter = 0;
	for (; iter < trantables->GetCount(); ++iter)
	{
		if (index == trantables->GetItem(iter)->typeId())
			break;
	}
	if ((table = trantables->GetItem(iter)) == NULL) {
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Not have this transfer table."
				 << "type:" << iter;
		return false;
	}
	if ((list = table->m_pVarAddrs) == NULL) {
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Not have this list.";
		return false;
	}
	return true;
}

CtrlField CIEC101::GetCurCtrlField (FC::CLIENT func)
{
	CtrlField tempcf;
	tempcf.dir = 1;
	tempcf.prm = 0;
	tempcf.fa  = !(m_obj->squeue1st.Empty());//linweiming
	tempcf.fc  = func;
	tempcf.fd  = 0;

	return tempcf;
}

void CIEC101::SendFixedFrame (const CtrlField& cfref)
{
	ST_BYTE sendData[32] = {0};
	Fixedframe* frame = (Fixedframe*) sendData;

	frame->start  = 0x10;
	frame->cf     = cfref;
	frame->laddr  = m_obj->option.laddr;
	frame->laddr1  = m_obj->option.laddr1;

	this->SendEx (sendData);
}

void CIEC101::KeepTotalCallConfirm ()
{
	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_IC_NA_1>* papdu = (APDU<TYP::C_IC_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2	    	= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2        	= papdu->apci.len1;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::C_IC_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::ACTCON;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;

	papdu->nsq[0].obj.qoi 		= QOI::GroupAll;

	this->SendEx(sendData);

	//m_obj->squeue1st.Push (sendData, sizeof (sendData));
}

void CIEC101::SendEqCallConfirm (int32_t qccfrz)
{
	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_CI_NA_1>* papdu = (APDU<TYP::C_CI_NA_1>*) sendData;

	papdu->apci.start1 	    	= APCI::STARTCHR;
	papdu->apci.start2 	    	= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2       		= papdu->apci.len1;
	papdu->apci.cf 				= GetCurCtrlField (FC::ANSWER_DATA);
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::C_CI_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::ACTCON;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;

	papdu->nsq[0].obj.rqt		= QCC::GroupAll;
	papdu->nsq[0].obj.frz		= qccfrz;

	this->SendEx (sendData);
}

void CIEC101::KeepCompareTimeComfirm (const CP56Time2a& ts)
{
	struct tm _tm;
	_tm.tm_sec   = ts.msec / 1000;
	_tm.tm_min   = ts.min;
	_tm.tm_hour  = ts.hour;
	_tm.tm_mday  = ts.mday;
	_tm.tm_mon   = ts.month - 1;
	_tm.tm_year  = ts.year  + 100;

	time_t timep = mktime(&_tm);
	struct timeval tv;
	tv.tv_sec    = timep;
	tv.tv_usec   = 0;

	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_CS_NA_1>* papdu = (APDU<TYP::C_CS_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2			= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2        	= papdu->apci.len1;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::C_CS_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	if(settimeofday (&tv, NULL) < 0) {
		papdu->asduh.cot.cause	= Cause::ACTTERM;
		cause = cause<<8 | Cause::ACTTERM;
	}
	else {
		papdu->asduh.cot.cause	= Cause::ACTCON;
		cause = cause<<8 | Cause::ACTCON;
	}

    papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;

	memcpy (&(papdu->nsq[0].obj.ts), &ts, sizeof (CP56Time2a));
    this->SendEx(sendData);
	//m_obj->squeue2st.Push (sendData, sizeof (sendData));
}

void CIEC101::KeepNAKComfirm (const uint8_t* data, int32_t cause)
{
	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	memcpy(sendData, data, Parser::GetSize(data));
	APDU<TYP::Undefined>* papdu = (APDU<TYP::Undefined>*) sendData;

	papdu->apci.cf              = GetCurCtrlField (FC::ANSWER_DATA);
	papdu->asduh.cot.pn         = true;
	papdu->asduh.cot.cause      = cause;

	m_obj->squeue2st.Push (sendData, sizeof (sendData));
}

void CIEC101::KeepInitEnded ()
{
	ST_BYTE sendData[32] = {0};
	APDU<TYP::M_EI_NA_1>* papdu = (APDU<TYP::M_EI_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2	    	= APCI::STARTCHR;

	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2       		= papdu->apci.len1;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::M_EI_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::INITIALIZE;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;
	papdu->nsq[0].obj.coi 		= COI::LocalPwOn;
    this->SendEx(sendData);
	//m_obj->squeue1st.Push (sendData, sizeof (sendData));
}

void CIEC101::KeepTotalCallEnded ()
{
	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_IC_NA_1>* papdu = (APDU<TYP::C_IC_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2	    	= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2       		= papdu->apci.len1;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::C_IC_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::ACTTERM;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;

	papdu->nsq[0].obj.qoi 		= QOI::GroupAll;
this->SendEx(sendData);
//	m_obj->squeue1st.Push (sendData, sizeof (sendData));
}

void CIEC101::KeepEqCallEnded ()
{
	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_CI_NA_1>* papdu = (APDU<TYP::C_CI_NA_1>*) sendData;

	papdu->apci.start1 			= APCI::STARTCHR;
	papdu->apci.start2 			= APCI::STARTCHR;
	// papdu->apci.cf 				= cfref;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2        	= papdu->apci.len1;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::C_CI_NA_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::ACTTERM;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;

	papdu->nsq[0].obj.rqt		= QCC::GroupAll;
	papdu->nsq[0].obj.frz		= QCC::Read;
this->SendEx(sendData);
//	m_obj->squeue2st.Push (sendData, sizeof (sendData));
}

void CIEC101::SendTier1Data ()
{
	CtrlField tempcf;
	tempcf.dir = 1;
	tempcf.prm = 0;
	tempcf.fd  = 0;

	tempcf.fa = (m_obj->squeue1st.Size() > 1 ? true : false);

	if (! (m_obj->squeue1st.Empty())) {
		tempcf.fc = FC::ANSWER_DATA;

		uint8_t* data = m_obj->squeue1st.Front ();
		((APCI*)data)->cf = tempcf;

		this->SendEx (data);
		m_obj->squeue1st.Pop ();

		m_obj->framesave.clear ();
		m_obj->framesave.insert (m_obj->framesave.begin(),
				data, data + Parser::GetSize(data));
		delete[] data;
	}
	else {
		tempcf.fc = FC::NO_ANSWER_DATA;
		SendFixedFrame (tempcf);
	}
}

void CIEC101::SendTier2Data ()
{
	if (m_obj->squeue2st.Empty()) {
		switch (m_obj->option.yc_type) {
			case TYP::M_ME_NC_1: CheckBWYC<TYP::M_ME_NC_1> ();
							 break;
			case TYP::M_ME_NB_1: CheckBWYC<TYP::M_ME_NB_1> ();
							 break;
			default:             CheckBWYC<TYP::M_ME_NA_1> ();
							 break;
		}
	}
	if (m_obj->squeue2st.Empty()) {
		SendFixedFrame (GetCurCtrlField (FC::NO_ANSWER_DATA));
	}
	else {
		uint8_t* data = m_obj->squeue2st.Front ();
		((APCI*)data)->cf = GetCurCtrlField (FC::ANSWER_DATA);

		this->SendEx (data);
		m_obj->squeue2st.Pop ();
		delete[] data;
	}
}

void CIEC101::CleanQueue ()
{
	while (! m_obj->squeue1st.Empty()) {
		delete[] m_obj->squeue1st.Front();
		m_obj->squeue1st.Pop();
	}
	while (! m_obj->squeue2st.Empty()) {
		delete[] m_obj->squeue2st.Front();
		m_obj->squeue2st.Pop();
	}
}

void CIEC101::TotalCallHelper (const uint8_t* data)
{
	SendFixedFrame (GetCurCtrlField (FC::CONFIRM));
	KeepTotalCallConfirm ();
	AllYXDataPacking     ();

	AllYCDataPacking<TYP::M_ME_NC_1> ();

	/*switch (m_obj->option.yc_type) {
		case TYP::M_ME_NC_1: AllYCDataPacking<TYP::M_ME_NC_1> ();
							 break;
		case TYP::M_ME_NB_1: AllYCDataPacking<TYP::M_ME_NB_1> ();
							 break;
		default:             AllYCDataPacking<TYP::M_ME_NA_1> ();
							 break;
	}*/
	KeepTotalCallEnded   ();
}

void CIEC101::EqCallHelper (const uint8_t* data)
{
	APDU<TYP::C_CI_NA_1>* papdu = (APDU<TYP::C_CI_NA_1>*) data;
	switch (papdu->nsq[0].obj.frz) {
		case QCC::Freeze : {
			SendEqCallConfirm (QCC::Freeze);
		} break;
		case QCC::Read   : {
			SendEqCallConfirm (QCC::Read);
			AllEqDataPacking  ();
			KeepEqCallEnded   ();
		} break;
	}
}
/*
//单点遥信上送
void CIEC101::AllYXDataPacking ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return ;
	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}

	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYP::M_SP_NA_1>* papdu = (APDU<TYP::M_SP_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2	    	= APCI::STARTCHR;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	//papdu->asduh.type0		    = 0x00;
	papdu->asduh.type		    = TYP::M_SP_NA_1;
	papdu->asduh.sq 		    = true;

	int16_t cause = 0;
	cause = cause<<8 | Cause::INROGEN;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;


	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh)
							 + sizeof ((papdu->sq).ioa);
	int16_t frame_size = cache_size;

	int32_t iter = 0;
	for (uint8_t msgcount = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			papdu->sq.ioa.set (iter + 0x0001);
			// papdu->sq.ioa.l16 = iter + 0x0001;
			// papdu->sq.ioa.h8  = (iter + 0x0001) / 65536;
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;
		ST_BOOLEAN bValue = stvar.bVal;
		if (TRVar && TRVar->Coefficient == -1.0) bValue = !bValue;

		((papdu->sq).obj[msgcount]).spi = bValue;

        m_datavalue[msgcount] = bValue;

		++msgcount;
		m_hv.add (iter + 0x0001, stvar);

		frame_size += sizeof (Type<TYP::M_SP_NA_1>);
		if (msgcount > 0x77 || iter + 1 >= list_size)
		{
			(papdu->apci ).len1	= frame_size - 4;
			(papdu->apci ).len2	= frame_size - 4;
			(papdu->asduh).num  = msgcount;

			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] TypeId: M_SP_NA_1, Cause: 20,"
					 << CDebug::bytesToHexString (sendData, frame_size);
this->SendEx(sendData);
			//m_obj->squeue1st.Push (sendData, sizeof (sendData));
			frame_size = cache_size;
			msgcount   = 0;
		}
	}
}
*/
//双点遥信上送
void CIEC101::AllYXDataPacking ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return ;
	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}

	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYP::M_DP_NA_1>* papdu = (APDU<TYP::M_DP_NA_1>*) sendData;

	papdu->apci.start1	    	= APCI::STARTCHR;
	papdu->apci.start2	    	= APCI::STARTCHR;
	papdu->apci.laddr       	= m_obj->option.laddr;
	papdu->apci.laddr1       	= m_obj->option.laddr1;
	//papdu->asduh.type0		    = 0x00;
	papdu->asduh.type		    = TYP::M_DP_NA_1;
	papdu->asduh.sq 		    = true;

	int16_t cause = 0;
	cause = cause<<8 | Cause::INROGEN;
	papdu->asduh.cot.cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	papdu->asduh.paddr 			= paddr;


	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh)
							 + sizeof ((papdu->sq).ioa);
	int16_t frame_size = cache_size;

	int32_t iter = 0;
	for (uint8_t msgcount = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			papdu->sq.ioa.set (iter + 0x0001);
			// papdu->sq.ioa.l16 = iter + 0x0001;
			// papdu->sq.ioa.h8  = (iter + 0x0001) / 65536;
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;
		ST_BOOLEAN bValue = stvar.bVal;
		if (TRVar && TRVar->Coefficient == -1.0) bValue = !bValue;

		((papdu->sq).obj[msgcount]).dpi = bValue?0x02:0x01;

        m_datavalue[msgcount] = bValue;

		++msgcount;
		m_hv.add (iter + 0x0001, stvar);

		frame_size += sizeof (Type<TYP::M_DP_NA_1>);
		if (msgcount > 0x77 || iter + 1 >= list_size)
		{
			(papdu->apci ).len1	= frame_size - 4;
			(papdu->apci ).len2	= frame_size - 4;
			(papdu->asduh).num  = msgcount;

			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] TypeId: M_SP_NA_1, Cause: 20,"
					 << CDebug::bytesToHexString (sendData, frame_size);
this->SendEx(sendData);
			//m_obj->squeue1st.Push (sendData, sizeof (sendData));
			frame_size = cache_size;
			msgcount   = 0;
		}
	}
}

template<int TYPE>
inline void InfoObjectFilling (Type<TYPE>& obj, float value, const Option& op)
{
	obj.mv = value;
}

template<>
inline void InfoObjectFilling<TYP::M_ME_NA_1>
	(Type<TYP::M_ME_NA_1>& obj, float value, const Option& op)
{
	int16_t tempval = (int16_t)(value * (32767.0 / op.fullcode));
	if (value < 0) {
		tempval = ((0x8001 - tempval) | 0x8000);
	}
	obj.ov = fabs(value) < 32767 ? false : true;
	obj.mv = *((uint16_t*)&tempval);
}

template<>
inline void InfoObjectFilling<TYP::M_ME_NB_1>
	(Type<TYP::M_ME_NB_1>& obj, float value, const Option& op)
{
	int16_t tempval = value;
	obj.ov = fabs(value) < 32767 ? false : true;
	obj.mv = (*(uint16_t*)&tempval);
}

template<int TYPE>
void CIEC101::AllYCDataPacking ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YCIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}

	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYPE>* papdu = (APDU<TYPE>*) sendData;

	(papdu->apci ).start1		= APCI::STARTCHR;
	(papdu->apci ).start2		= APCI::STARTCHR;
	(papdu->apci ).laddr        = m_obj->option.laddr;
	(papdu->apci ).laddr1        = m_obj->option.laddr1;
	(papdu->asduh).type			= TYPE;
	(papdu->asduh).sq 			= true;
	int16_t cause = 0;
	cause = cause<<8 | Cause::INROGEN;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh)
							 + sizeof (papdu->sq.ioa);
	int16_t frame_size = cache_size;

	int32_t iter = 0;
	int16_t index = 0;
	for (uint8_t msgcount = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			papdu->sq.ioa.set (iter + 0x4001);
			// (papdu->sq).ioa.l16 = iter + 0x4001;
			// (papdu->sq).ioa.h8  =(iter + 0x4001) / 63356;
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_FLOAT tempvar = 0.0;
		if (TRVar) {
			if (TRVar->Coefficient == 0) TRVar->Coefficient = 1;
			tempvar = stvar.fVal * TRVar->Coefficient;
		}
		// ((papdu->sq).obj[msgcount]).mv = tempvar;
		m_fdatavalue[index++] = tempvar;
		InfoObjectFilling<TYPE> ((papdu->sq).obj[msgcount], tempvar, m_obj->option);
		m_hv.add (iter + 0x4001, stvar);
		++msgcount;

		frame_size += sizeof (Type<TYPE>);
		if (frame_size + (int16_t)sizeof(Type<TYPE>) > APCI::MAX_FRAME_SIZE - 3
								 || iter + 1 >= list_size)
		{
			(papdu->apci ).len1	= frame_size - 4;
			(papdu->apci ).len2	= frame_size - 4;
			(papdu->asduh).num  = msgcount;
this->SendEx(sendData);
	//		m_obj->squeue1st.Push (sendData, sizeof (sendData));
			frame_size = cache_size;
			msgcount   = 0;
		}
	}
}

void CIEC101::AllEqDataPacking ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YMIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}
	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYP::M_IT_NA_1>* papdu = (APDU<TYP::M_IT_NA_1>*) sendData;

	(papdu->apci ).start1		= APCI::STARTCHR;
	(papdu->apci ).start2		= APCI::STARTCHR;
	(papdu->apci ).laddr        = m_obj->option.laddr;
	(papdu->apci ).laddr1        = m_obj->option.laddr1;
	(papdu->asduh).type			= TYP::M_IT_NA_1;
	(papdu->asduh).sq 			= true;

    int16_t cause = 0;
	cause = cause<<8 | Cause::INROGEN;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh)
							 + sizeof (papdu->sq.ioa);
	int16_t frame_size = cache_size;

	uint8_t msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			papdu->sq.ioa.set (iter + 0x6401);
			// (papdu->sq).ioa.l16 = iter + 0x6401;
			// (papdu->sq).ioa.h8  = (iter + 0x6401) / 65536;
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		int32_t tempvar = 0;
		if (TRVar) {
			if (TRVar->Coefficient == 0.0)
				TRVar->Coefficient = 1;
			if (stvar.vt == VALType_Double)
				tempvar = static_cast<int32_t>(stvar.dVal * TRVar->Coefficient);
			else
				tempvar = static_cast<int32_t>(stvar.fVal * TRVar->Coefficient);
		}

		((papdu->sq).obj[msgcount]).mv = tempvar;
		++msgcount;

		frame_size += sizeof (Type<TYP::M_IT_NA_1>);
		if (frame_size + (short)sizeof(Type<TYP::M_IT_NA_1>) > APCI::MAX_FRAME_SIZE -3
								 || iter + 1 >= list_size)
		{
			(papdu->apci ).len1	= frame_size - 4;
			(papdu->apci ).len2	= frame_size - 4;
			(papdu->asduh).num  = msgcount;
this->SendEx(sendData);
			//m_obj->squeue2st.Push (sendData, sizeof (sendData));

			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] TypeId: M_IT_NA_1, Cause: 20,"
					 << CDebug::bytesToHexString (sendData, frame_size);

			frame_size = cache_size;
			msgcount   = 0;
		}
	}
}

void CIEC101::CheckBWYX ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}

	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYP::M_SP_NA_1>* papdu = (APDU<TYP::M_SP_NA_1>*) sendData;

	papdu->apci.start1			= APCI::STARTCHR;
	papdu->apci.start2			= APCI::STARTCHR;
	papdu->apci.laddr        	= m_obj->option.laddr;
	papdu->apci.laddr1        	= m_obj->option.laddr1;
	papdu->asduh.type			= TYP::M_SP_NA_1;
	papdu->asduh.sq 			= false;

    int16_t cause = 0;
	cause = cause<<8 | Cause::SPONTANEOUS;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh);
	int16_t frame_size = cache_size;

	int32_t& iter = m_obj->YXiter;
	if (iter + 1 >= list_size) iter = 0;
	int16_t iIndex = 0;
	for (uint8_t msgcount = 0; iter < list_size; ++iter)
	{
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_VARIANT* oldstvar = m_hv.find (iter + 0x0001);
	//	if (oldstvar && oldstvar->bVal != stvar.bVal) {

			ST_BOOLEAN bValue = stvar.bVal;
			if (TRVar && TRVar->Coefficient == -1.0) bValue = !bValue;
			(papdu->nsq[msgcount]).obj.spi   = bValue;

			if(m_datavalue[iIndex] != bValue)
			{
                (papdu->nsq[msgcount]).ioa.set (iter + 0x0001);
                m_datavalue[iIndex] = bValue;
                m_hv.add (iter + 0x0001, stvar);

                ++msgcount;
         //       frame_size += sizeof (Type<TYP::M_SP_NA_1>);
                frame_size += sizeof (papdu->nsq[0]);
			}
			iIndex++;
			// (papdu->nsq[msgcount]).ioa.h8  = (iter + 0x0001) / 65536;
			// (papdu->nsq[msgcount]).ioa.l16 = (iter + 0x0001) % 65536;

	//	}

		if (msgcount && (msgcount > 0x77 || iter + 1 >= list_size))
		{
			papdu->apci.len1	= frame_size - 4;
			papdu->apci.len2	= frame_size - 4;
			papdu->asduh.num    = msgcount;

			sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] TypeId: M_SP_NA_1, Cause: 3,"
					 << CDebug::bytesToHexString (sendData, frame_size);
            this->SendEx(sendData);
		//	m_obj->squeue1st.Push (sendData, sizeof (sendData));
			frame_size = cache_size;
			break;
		}
	}
}

template<int TYPE>
void CIEC101::CheckBWYC ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YCIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		return ;
	}

	ST_BYTE sendData[APCI::MAX_FRAME_SIZE] = {0};
	APDU<TYPE>* papdu = (APDU<TYPE>*) sendData;

	papdu->apci.start1			= APCI::STARTCHR;
	papdu->apci.start2			= APCI::STARTCHR;
	papdu->apci.laddr        	= m_obj->option.laddr;
	papdu->apci.laddr1        	= m_obj->option.laddr1;
	papdu->asduh.type			= TYPE;
	papdu->asduh.sq 			= false;

	int16_t cause = 0;
	cause = cause<<8 | Cause::BGSCAN;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

	const int16_t cache_size = sizeof (papdu->apci) + sizeof (papdu->asduh);
	int16_t frame_size  = cache_size;

	int32_t& iter = m_obj->YCiter;
	if (iter + 1 >= list_size) iter = 0;
	int16_t nIndex = 0;
	for (uint8_t msgcount = 0; iter < list_size; ++iter)
	{
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_VARIANT * oldstvar = m_hv.find (iter + 0x4001);
	//	if (oldstvar && fabs (oldstvar->fVal - stvar.fVal) > 0.012)
		{
			if (TRVar && TRVar->Coefficient == 0)
				TRVar->Coefficient = 1;

			ST_FLOAT tempvar = stvar.fVal * TRVar->Coefficient;
			// papdu->nsq[msgcount].obj.mv  = tempvar;
			InfoObjectFilling<TYPE> (papdu->nsq[msgcount].obj, tempvar, m_obj->option);
//			papdu->nsq[msgcount].ioa.h8  = (iter + 0x4001) / 65536;
//			papdu->nsq[msgcount].ioa.l16 = (iter + 0x4001) % 65536;
			(papdu->nsq[msgcount]).ioa.set (iter + 0x4001);
            if(fabs( (m_fdatavalue[nIndex] - stvar.fVal)/m_fdatavalue[nIndex]) > 0.1)
            {
                m_hv.add (iter + 0x4001, stvar);
                m_fdatavalue[nIndex] = stvar.fVal;
                ++msgcount;
                frame_size += sizeof (papdu->nsq[0]);
            }
            nIndex++;
		}

		if (msgcount && (frame_size + (short)sizeof(papdu->nsq[0]) > APCI::MAX_FRAME_SIZE -3
								|| iter + 1 >= list_size))
		{
			papdu->apci.len1	= frame_size - 4;
			papdu->apci.len2	= frame_size - 4;
			papdu->asduh.num    = msgcount;
this->SendEx(sendData);
		//	m_obj->squeue2st.Push (sendData, sizeof (sendData));
			frame_size = cache_size;
			break;
		}
	}
}

void CIEC101::ForwardTellSOE (ProtocolTask& task)
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;
	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return;

	int32_t list_size = tablelist->GetCount();
	int32_t iter = 0;
	for (; iter < list_size; ++iter) {
		ST_DUADDR* pda = tablelist->GetItem(iter);
		if (task.taskAddr == pda->addr
			&& task.taskAddr1 == pda->device)
			break;
	}
	if (iter == list_size)
	{
        ShowMessage("NOt Found point");
        return;
	}


	ST_BYTE sendData[32] = {0};
	APDU<TYP::M_SP_TB_1>* papdu = (APDU<TYP::M_SP_TB_1>*) sendData;

	papdu->apci.start1			= APCI::STARTCHR;
	papdu->apci.start2			= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2			= papdu->apci.len1;
	papdu->apci.laddr        	= m_obj->option.laddr;
	papdu->apci.laddr1        	= m_obj->option.laddr1;
	papdu->asduh.type 			= TYP::M_SP_TB_1;
	papdu->asduh.num 			= 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::SPONTANEOUS;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

//	papdu->nsq[0].ioa.h8		= (iter + 0x0001) / 65536;
//  papdu->nsq[0].ioa.l16		= (iter + 0x0001) % 65536;
	papdu->nsq[0].ioa.set (iter + 0x0001);
	papdu->nsq[0].obj.spi 		= task.taskValue;
	papdu->nsq[0].obj.ts.msec	= task.taskParam[7] + task.taskParam[8] * 256
								+ task.taskParam[6] * 1000; //msecond
	papdu->nsq[0].obj.ts.min	= task.taskParam[5] & 0x3F;
	papdu->nsq[0].obj.ts.hour	= task.taskParam[4] & 0x1F;
	papdu->nsq[0].obj.ts.mday	= task.taskParam[3] & 0x1F; //day
	papdu->nsq[0].obj.ts.month  = task.taskParam[2] & 0x0F; //month
	papdu->nsq[0].obj.ts.year	=(task.taskParam[0] + task.taskParam[1] * 256 - 2000) & 0x7F; //year
this->SendEx(sendData);
//	m_obj->squeue1st.Push (sendData, sizeof (sendData));

	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "][Forward tell SOE.]";

	memset (&task, 0, sizeof (task));
}

void CIEC101::TransC_SC_NA_1ToLower (const ST_BYTE* data)
{
	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Check C_SC_NA_1 Dit.";

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
	{
		ShowMessage("CheckTransferTableExist");
		return ;
    }

	int32_t list_size = tablelist->GetCount();

	APDU<TYP::C_SC_NA_1>* papdu = (APDU<TYP::C_SC_NA_1>*) data;
	// int32_t iter = (papdu->nsq[0].ioa.h8 * 65536 + papdu->nsq[0].ioa.l16) - 0x6001;
	int32_t iter = papdu->sq.ioa.get () - 0x6001;
	if (list_size <= 0 || list_size < iter || iter < 0) {
		KeepNAKComfirm (data, Cause::UNKNOWNINFOADDR);
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] UNKNOWNINFOADDR: " << iter;
		return ;
	}

	ST_DUADDR* pda = tablelist->GetItem(iter);

	ProtocolTask *task 		= new ProtocolTask;
	task->isTransfer 		= true;
	task->transChannelId 	= -1;
	task->transDeviceId 	= pda->device;
	task->taskAddr 			= pda->addr;
	task->taskValue 		= papdu->nsq[0].obj.scs;

	switch (papdu->asduh.cot.cause) {
		case Cause::ACT  : {
			task->taskCmdCode = (papdu->nsq[0].obj.se ? 0x00 : 0x01 );

		} break;

		case Cause::DEACT: {
			task->taskCmdCode = 0x02;
		} break;
	}
	strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));
	Transfer(task);
	delete task;
}

void CIEC101::TransC_SC_NA_1ToUpper (const ProtocolTask& task)
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;
	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
		return;

	int32_t list_size = tablelist->GetCount();
	int32_t iter = 0;
	for (; iter < list_size; ++iter) {
		ST_DUADDR* pda = tablelist->GetItem(iter);
		if (task.taskAddr == pda->addr
			&& task.transDeviceId == pda->device)
			break;
	}
	if (iter == list_size)
		return;

	iter += 0x6001;

	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Part C_SC_NA_1 task.";

	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_SC_NA_1>* papdu = (APDU<TYP::C_SC_NA_1>*) sendData;

	papdu->apci.start1 			= APCI::STARTCHR;
	papdu->apci.start2 			= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2            = papdu->apci.len1;
	papdu->apci.laddr        	= m_obj->option.laddr;
	papdu->asduh.type 			= TYP::C_SC_NA_1;
	papdu->asduh.num 			= 0x01;
	papdu->asduh.paddr 			= m_obj->option.laddr;
	// papdu->nsq[0].ioaddr8h	= iter / 65536;
	// papdu->nsq[0].ioa.l16    = iter % 65536;
	papdu->nsq[0].ioa.set (iter);
	papdu->nsq[0].obj.scs		= task.taskValue;

	switch (task.taskCmdCode) {
		case 0x00 : papdu->nsq[0].obj.se = 0x1;
					papdu->asduh.cot.cause = Cause::ACTCON;
					break;
		case 0x01 : papdu->nsq[0].obj.se = 0x0;
					papdu->asduh.cot.cause = Cause::ACTCON;
					break;
		case 0x02 : papdu->nsq[0].obj.se = 0x0;
					papdu->asduh.cot.cause = Cause::DEACTCON;
					break;
	}
this->SendEx(sendData);
	//m_obj->squeue1st.Push (sendData, sizeof (sendData));
}

void CIEC101::TransC_DC_NA_1ToLower (const ST_BYTE* data)
{
	APDU<TYP::C_DC_NA_1>* papdu = (APDU<TYP::C_DC_NA_1>*) data;
	if (papdu->nsq[0].obj.dcs == 0x00 || papdu->nsq[0].obj.dcs == 0x03) {
		return ;
	}

	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Check C_DC_NA_1 Dit.";

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();

	// int32_t iter = (papdu->nsq[0].ioa.h8 * 65536 + papdu->nsq[0].ioa.l16) - 0x6001;
	int32_t iter = papdu->nsq[0].ioa.get () - 0x6001;
	if (list_size <= 0 || list_size < iter || iter < 0) {
		sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] UNKNOWNINFOADDR: " << iter;
		KeepNAKComfirm (data, Cause::UNKNOWNINFOADDR);
		return ;
	}
	ST_DUADDR* pda = tablelist->GetItem(iter);

	sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "] Part C_DC_NA_1 task.";

	ProtocolTask *task 		= new ProtocolTask;
	task->isTransfer 		= true;
	task->transChannelId 	= -1;
	task->transDeviceId 	= pda->device;
	task->taskAddr 			= pda->addr;
	task->taskValue 		= (papdu->nsq[0].obj.dcs == 0x02 ? 0x01 : 0x00);
	switch (papdu->asduh.cot.cause) {
		case Cause::ACT  : {
			task->taskCmdCode = (papdu->nsq[0].obj.se ? 0x00 : 0x01);
		} break;

		case Cause::DEACT: {
			task->taskCmdCode = 0x02;
		} break;
	}
	strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));

	Transfer(task);
	delete task;
}

void CIEC101::TransC_DC_NA_1ToUpper (const ProtocolTask& task)
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;
	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
		return;

	int32_t list_size = tablelist->GetCount();
	int32_t iter = 0;
	for (; iter < list_size; ++iter) {
		ST_DUADDR* pda = tablelist->GetItem(iter);
		if (task.taskAddr == pda->addr
			&& task.transDeviceId == pda->device)
			break;
	}
	if (iter == list_size)
		return;

	iter += 0x6001;

	ST_BYTE sendData[32] = {0};
	APDU<TYP::C_DC_NA_1>* papdu = (APDU<TYP::C_DC_NA_1>*) sendData;

	papdu->apci.start1		 	= APCI::STARTCHR;
	papdu->apci.start2		 	= APCI::STARTCHR;
	papdu->apci.len1			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq[0]) - 4;
	papdu->apci.len2            = papdu->apci.len1;
	papdu->apci.laddr        	= m_obj->option.laddr;
	papdu->asduh.type 			= TYP::Undefined;
	papdu->asduh.num 			= 0x01;
	papdu->asduh.paddr 			= m_obj->option.laddr;
	// papdu->nsq[0].ioa.h8		= iter / 65536;
	// papdu->nsq[0].ioa.l16  	= iter % 65536;
	ShowMessage("1466");
	papdu->nsq[0].ioa.set (iter);
	papdu->nsq[0].obj.dcs		= (task.taskValue == 0x1 ? 0x2 : 0x1);

	switch (task.taskCmdCode) {
		case 0x00 : papdu->nsq[0].obj.se    = 0x1;
					papdu->asduh.cot.cause = Cause::ACTCON;
					break;
		case 0x01 : papdu->nsq[0].obj.se    = 0x0;
					papdu->asduh.cot.cause = Cause::ACTCON;
					break;
		case 0x02 : papdu->nsq[0].obj.se    = 0x0;
					papdu->asduh.cot.cause = Cause::DEACTCON;
					break;
	}
//this->SendEx(sendData);
	m_obj->squeue1st.Push (sendData, sizeof (sendData));
}

void CIEC101::TaskHandlerToLower (const uint8_t* data)
{
/*
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return ;
    int32_t list_size = tablelist->GetCount();
    if (list_size <= 0) {
        return ;
    }
    ST_DUADDR  *duaddr = tablelist->GetItem(32);  //需变动，最好弄成就地位置数组
    TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
    ST_VARIANT stvar;
    ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
    if (nR < 0) stvar.ulVal = 0;
    ST_BOOLEAN bValue = stvar.bVal;
    if (TRVar && TRVar->Coefficient == -1.0) bValue = !bValue;
    if(bValue)
    {*/
        if(data[15]>=0x80)
        {//68 0C 0C 68 73 86 08 2E 01 06 00 86 08 01 60 82 A7 16
           ShowMessage ("11771.");
          // sleep(1500);
           ST_BYTE sendbuf[32];
           sendbuf[0] = 0x68;
           sendbuf[1] = 0x0C;
           sendbuf[2] = 0x0C;
           sendbuf[3] = 0x68;
           sendbuf[4] = 0x80;//0x83;

           sendbuf[5] = m_obj->option.laddr;
           sendbuf[6] = m_obj->option.laddr1;

           sendbuf[7] = data[7];
           sendbuf[8] = 0x01;
           sendbuf[9] = 0x07;
           sendbuf[10] = 0x00;
           sendbuf[11] = m_obj->option.laddr;
           sendbuf[12] = m_obj->option.laddr1;
           sendbuf[13] = data[13];
           sendbuf[14] = data[14];
           sendbuf[15] = data[15];
           ST_BYTE sum = 0x00;
           int16_t ii = 0;
           for(ii=4;ii<16;ii++)
            sum += sendbuf[ii];
           sendbuf[16] = sum;
           sendbuf[17] = 0x16;
           this->Send(sendbuf,18);
        }
        else if((data[15] == 0x01) || (data[15] == 0x02))
        {
        //遥控执行确认
           ShowMessage ("1772.");
           ST_BYTE sendbuf[32];
           sendbuf[0] = 0x68;
           sendbuf[1] = 0x0C;
           sendbuf[2] = 0x0C;
           sendbuf[3] = 0x68;
           sendbuf[4] = 0x80;//0x83;

           sendbuf[5] = m_obj->option.laddr;
           sendbuf[6] = m_obj->option.laddr1;

           sendbuf[7] = data[7];
           sendbuf[8] = 0x01;
           sendbuf[9] = 0x07;
           sendbuf[10] = 0x00;
           sendbuf[11] = m_obj->option.laddr;
           sendbuf[12] = m_obj->option.laddr1;
           sendbuf[13] = data[13];
           sendbuf[14] = data[14];
           sendbuf[15] = data[15];
           ST_BYTE sum1 = 0x00;
           int16_t ii = 0;
           for(ii=4;ii<16;ii++)
            sum1 += sendbuf[ii];
           sendbuf[16] = sum1;
           sendbuf[17] = 0x16;
           this->Send(sendbuf,18);

          //遥控执行终止
          //  68 0C 0C 68 73 86 08 2D 01 06 00 86 08 01 60 81 A5 16
           ShowMessage ("1772.");
           ST_BYTE sendbuf1[32];
           sendbuf1[0] = 0x68;
           sendbuf1[1] = 0x0C;
           sendbuf1[2] = 0x0C;
           sendbuf1[3] = 0x68;
           sendbuf1[4] = 0x80;//0x83;

           sendbuf1[5] = m_obj->option.laddr;
           sendbuf1[6] = m_obj->option.laddr1;

           sendbuf1[7] = data[7];
           sendbuf1[8] = 0x01;
           sendbuf1[9] = 0x0A;
           sendbuf1[10] = 0x00;
           sendbuf1[11] = m_obj->option.laddr;
           sendbuf1[12] = m_obj->option.laddr1;
           sendbuf1[13] = data[13];
           sendbuf1[14] = data[14];
           sendbuf1[15] = data[15];
           ST_BYTE sum2 = 0x00;
           int16_t k = 0;
           for(k=4;k<16;k++)
            sum2 += sendbuf1[k];
           sendbuf1[16] = sum2;
           sendbuf1[17] = 0x16;
           this->Send(sendbuf1,18);
        }
        taskexpire.start ();

        APDU<TYP::Undefined>* papdu = (APDU<TYP::Undefined>*) data;
        m_iecTaskType = papdu->asduh.type;
        switch (m_iecTaskType) {
            case TYP::C_SC_NA_1: {   // 单点命令
                TransC_SC_NA_1ToLower (data);
            } break;
            case TYP::C_DC_NA_1: {   // 双点命令
                TransC_DC_NA_1ToLower (data);
            } break;
            case TYP::C_RC_NA_1: {   //
            } break;
            case TYP::C_SE_NA_1: {   //
            } break;
            case TYP::C_SE_NB_1: {   //
            } break;
            case TYP::C_SE_NC_1: {   //
            } break;
        }
 /*   }
    else
    {
        if(data[15]>=0x80)
        {//68 0C 0C 68 73 86 08 2E 01 06 00 86 08 01 60 82 A7 16
           ShowMessage ("11771.");
          // sleep(1500);
           ST_BYTE sendbuf[32];
           sendbuf[0] = 0x68;
           sendbuf[1] = 0x0C;
           sendbuf[2] = 0x0C;
           sendbuf[3] = 0x68;
           sendbuf[4] = 0x80;//0x83;

           sendbuf[5] = m_obj->option.laddr;
           sendbuf[6] = m_obj->option.laddr1;

           sendbuf[7] = data[7];
           sendbuf[8] = 0x01;
           sendbuf[9] = 0x47;
           sendbuf[10] = 0x00;
           sendbuf[11] = m_obj->option.laddr;
           sendbuf[12] = m_obj->option.laddr1;
           sendbuf[13] = data[13];
           sendbuf[14] = data[14];
           sendbuf[15] = data[15];
           ST_BYTE sum = 0x00;
           int16_t ii = 0;
           for(ii=4;ii<16;ii++)
            sum += sendbuf[ii];
           sendbuf[16] = sum;
           sendbuf[17] = 0x16;
           this->Send(sendbuf,18);
        }
        else if((data[15] == 0x01) || (data[15] == 0x02))
        {
        //遥控执行确认
           ShowMessage ("1772.");
           ST_BYTE sendbuf[32];
           sendbuf[0] = 0x68;
           sendbuf[1] = 0x0C;
           sendbuf[2] = 0x0C;
           sendbuf[3] = 0x68;
           sendbuf[4] = 0x80;//0x83;

           sendbuf[5] = m_obj->option.laddr;
           sendbuf[6] = m_obj->option.laddr1;

           sendbuf[7] = data[7];
           sendbuf[8] = 0x01;
           sendbuf[9] = 0x47;
           sendbuf[10] = 0x00;
           sendbuf[11] = m_obj->option.laddr;
           sendbuf[12] = m_obj->option.laddr1;
           sendbuf[13] = data[13];
           sendbuf[14] = data[14];
           sendbuf[15] = data[15];
           ST_BYTE sum1 = 0x00;
           int16_t ii = 0;
           for(ii=4;ii<16;ii++)
            sum1 += sendbuf[ii];
           sendbuf[16] = sum1;
           sendbuf[17] = 0x16;
           this->Send(sendbuf,18);
        }


    }*/

}

void CIEC101::TaskHandlerToUpper (ProtocolTask& task)
{

	switch (m_iecTaskType) {
		case TYP::C_SC_NA_1: {   // 单点命令
		    TransC_SC_NA_1ToUpper (task);
		} break;
		case TYP::C_DC_NA_1: {   // 双点命令
		    TransC_DC_NA_1ToUpper (task);
		} break;
		case TYP::C_RC_NA_1: {   //
		} break;
		case TYP::C_SE_NA_1: {   //
		} break;
		case TYP::C_SE_NB_1: {   //
		} break;
		case TYP::C_SE_NC_1: {   //
		} break;
	}

	memset (&task, 0, sizeof (task));
	m_iecTaskType = TYP::Undefined;
	ShowMessage("1627");
}

//For Deleting...
void CIEC101::AccidentsHandler (int32_t tableIndex, int32_t addr)
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (tableIndex, trantable, tablelist))
        return ;

    int32_t sglist_size = tablelist->GetCount();
    if (sglist_size <= 0) {
        return ;
    }

    int32_t sg_iter = 0;
    ST_VARIANT stvar;
    ST_BOOLEAN bValue;
    for (; sg_iter < sglist_size; ++sg_iter) {
        stvar.ulVal = 0;
        bValue = false;
        ST_DUADDR  *duaddr = tablelist->GetItem(sg_iter);
        TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
        ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
        if (nR < 0) stvar.ulVal = 0;
        bValue = stvar.bVal;
        if (TRVar && TRVar->Coefficient == -1.0)
            bValue = !bValue;

        if (bValue == true)
            break;
    }

    this->UpdateValue (addr, (sg_iter < sglist_size ? true : false));

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return;

    int32_t yxlist_size = tablelist->GetCount();
    int32_t iter = 0;
    for (; iter < yxlist_size; ++iter) {
        ST_DUADDR* pda = tablelist->GetItem(iter);
        if (addr == pda->addr
            && this->GetDevice()->GetDeviceInfo()->DeviceId == pda->device)
            break;
    }
    if (iter == yxlist_size)
        return;

    ST_VARIANT* oldvar = m_hv.find (iter + 0x0001);
    if (!oldvar || oldvar->bVal == stvar.bVal)
        return;

    sDebug() << "[" << GetDevice()->GetDeviceInfo()->DeviceName << "][SGTotal change.] bool:" << stvar.ulVal
             << ", size:"  << yxlist_size << ", iter:"  << iter;

    m_hv.add (iter + 0x0001, stvar);

    ST_BYTE sendData[32] = {0};
    APDU<TYP::M_SP_TB_1>* papdu = (APDU<TYP::M_SP_TB_1>*) sendData;

    papdu->apci.start1        	= APCI::STARTCHR;
    papdu->apci.start2        	= APCI::STARTCHR;
    papdu->apci.len1          	= sizeof (papdu->apci) + sizeof (papdu->asduh)
                                + sizeof (papdu->nsq[0]) - 4;
    papdu->apci.len2            = papdu->apci.len1;
    papdu->apci.laddr        	= m_obj->option.laddr;
    papdu->apci.laddr1        	= m_obj->option.laddr1;
    papdu->asduh.type           = TYP::M_SP_TB_1;
    papdu->asduh.num            = 0x01;

    int16_t cause = 0;
	cause = cause<<8 | Cause::SPONTANEOUS;
	(papdu->asduh.cot).cause 		= cause;
	int16_t paddr = m_obj->option.laddr1;
	paddr = paddr<<8 | m_obj->option.laddr;
	(papdu->asduh).paddr 			= paddr;

    struct tm tm_now;
    DateTime::localtime (time(0), tm_now);

    papdu->nsq[0].ioa.set (iter + 0x0001);
    papdu->nsq[0].obj.spi       = bValue;
    papdu->nsq[0].obj.ts.msec   = tm_now.tm_sec  * 999 ;   //second
    papdu->nsq[0].obj.ts.min    = tm_now.tm_min  & 0x3F;
    papdu->nsq[0].obj.ts.hour   = tm_now.tm_hour & 0x1F;
    papdu->nsq[0].obj.ts.mday   = tm_now.tm_mday & 0x1F;   //day
    papdu->nsq[0].obj.ts.wday   = tm_now.tm_wday & 0x07;
    papdu->nsq[0].obj.ts.month  = tm_now.tm_mon  + 1;      //month
    papdu->nsq[0].obj.ts.year   = tm_now.tm_year - 100;    //year

    m_obj->squeue1st.Push (sendData, sizeof (sendData));
}
