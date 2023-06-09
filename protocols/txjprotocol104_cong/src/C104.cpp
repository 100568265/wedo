

#include "C104.h"
#include "sysmutex.h"
#include "IEC_101or104_types.h"
#include "Debug.h"
#include "FakeTimer.h"
#include "datetime.h"
#include <cmath>
#include <vector>

volatile time_t FakeTimer::curr_sec = 0;

#define wDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote
#define logWarn     SysLogger::GetInstance()->LogWarn
#define lDebug		if(false)wedoDebug().noquote
// extern NodeTree *g_pTree;

class IECFrameCache
{
public:
	typedef std::vector<iec104_apdu*>::iterator iterator;

	IECFrameCache () {}
	~IECFrameCache () {
		clear ();
	}
	void add (iec104_apdu* papdu) {
		iec104_apdu* ptr = new iec104_apdu;
		memcpy(ptr, papdu, papdu->apci.length + 2);

		Locker locker(&(this->_mutex));
		v.push_back (ptr);
	}

	iec104_apdu* at (uint32_t i) const {
		if (v.size() <= i || !v.size())
			return NULL;

		return v[i];
	}

	uint32_t size () const { return v.size (); }

	void clear () {
		Locker locker(&(this->_mutex));

		for (iterator it = v.begin(); it != v.end (); ++it) {
			delete *it;
		}
		v.clear ();
	}
private:
	std::vector<iec104_apdu*> v;
	Mutex _mutex;
};

class IEC104Obj
{
 public:
	IEC104Obj() : isStart(false), t2(10), t3(30) { init(); }
	~IEC104Obj() {}

	void init () { wCount = VS = VR = 0; }

	static const unsigned int STARTCHR 		= 0x68;
	static const unsigned int SUPERVISORY 	= 0x01;
	static const unsigned int STARTDTACT 	= 0x07;
	static const unsigned int STARTDTCON 	= 0x0B;
	static const unsigned int STOPDTACT 	= 0x13;
	static const unsigned int STOPDTCON 	= 0x23;
	static const unsigned int TESTFRACT 	= 0x43;
	static const unsigned int TESTFRCON 	= 0x83;

	static const unsigned short W_DEFAULT   = 0x08;
	static const unsigned short K_DEFAULT   = 0x0C;

	static const short MaxFrameSize  = 0xFF;

	volatile unsigned short VS;
	volatile unsigned short VR;

	unsigned short wCount;	//

	volatile unsigned short PAddr;

	volatile bool isStart;

	IECFrameCache cache;

	FakeTimer t2;
	FakeTimer t3;
}; // ! class IEC104Obj

C104* CreateInstace()
{
	return new C104 ();
}

C104::C104() : taskexpire (10), m_obj(new IEC104Obj())
{
	//ctor
	wDebug() << "[Debug][" << (void*)this << "] IEC104 Protocol Constructed";
}

C104::~C104()
{
	//dtor
	delete m_obj;
	wDebug() << "[Debug][" << (void*)this << "] IEC104 Protocol Unconstructed";
}

void C104::Init ()
{
	m_bIsRun      = false;
	m_nCallStep   = 0;
	m_nEQCallStep = 0;
	m_iecTaskType = Undefined;

	wDebug() << "[Debug][" << (void*)this << "] IEC104 Protocol Initialized";
}

void C104::Uninit()
{
	wDebug() << "[Debug][" << (void*)this << "] IEC104 Protocol Uninitialized";
}

void C104::OnRead (ST_BYTE* pbuf, ST_INT& readed)
{
	FakeTimer::timing ();

	readed = 0;
	do {
		if (! this->IsOpened()) {
			break;
		}

		if(! this->GetCurPort ()) {
			break;
		}

		int32_t	len = this->GetCurPort()->PickBytes(pbuf, 6, 1000);
		if(len < 6) {
			this->GetCurPort()->Clear();
			break;
		}

		int32_t star = 0;
		for(; star < len; star++)
		{
			if(pbuf[star] == IEC104Obj::STARTCHR)
				break;
		}
		if(star == len)
		{
			wDebug() << "[Debug][" << (void*)this << "] All Data is messy code to clear,\n SurPacket :"
					 <<  CDebug::bytesToHexString (pbuf, len);
			this->GetCurPort()->Clear();
			break;
		}
		if(star > 0)
		{
			wDebug() << "[Debug][" << (void*)this << "] Part Data is messy code to clear,\n SurPacket :"
					 <<  CDebug::bytesToHexString (pbuf, len);
			this->GetCurPort()->ReadBytes(pbuf, star);
		}

		len = this->GetCurPort()->PickBytes(pbuf, 6, 1000);
		int32_t ndatalen = ((iec104_apci*) pbuf)->length + 2;

		len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
		if(len == ndatalen) {
			readed = len;
			break;
		}
		else {
			wDebug() << "[Debug][" << (void*)this << "] Data Length is under to clear,\n SurPacket :"
					 <<  CDebug::bytesToHexString (pbuf, len);
			this->GetCurPort()->Clear();
		}
	} while (0);

	FakeTimer::timing ();

	if (! m_bIsSendSuc) {
		m_obj->init ();
		m_obj->isStart = false;
		m_bIsRun = false;
	}

	if (m_obj->t2.isTimeout()) {
		m_obj->t2.stop ();
		wDebug() << "[Debug][" << (void*)this << "][t2 time out.]";
		SendMonitorFrame ();
	}

	if (m_obj->t3.isTimeout()) {
		m_obj->t3.stop ();
		wDebug() << "[Debug][" << (void*)this << "][t3 time out.]";
		SendTestFrame ();
	}

	if (taskexpire.isTimeout()) {
		taskexpire.stop ();
		wDebug() << "[Debug][" << (void*)this << "][task time out.]";
		m_iecTaskType = Undefined;
	}
}

ST_BOOLEAN C104::OnSend ()
{
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"SOE"))
		{
			wDebug() << "[Debug][" << (void*)this << "][Has SOE.]";
			ForwardTellSOE (m_curTask);
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			TaskHandlerToUpper (m_curTask);
			return true;
		}
		else return false;
	}

	if (m_nCallStep) {
		switch (m_nCallStep) {
			case 1:
                SendTotalCallConfirm ();
				m_nCallStep = 2;
				return true;

			case 2:
                SendAllYXData ();
				m_nCallStep = 3;
				return true;

			case 3:
			    SendAllYCData ();
				m_nCallStep = 4;
				return true;

			case 4:
			    Thread::SLEEP (50);
				SendTotalCallEnded ();
				m_nCallStep = 0;

				m_bIsRun = true;
				return true;
		}
	}

	if (m_nEQCallStep) {
		switch (m_nEQCallStep) {
			case 1:
				SendEqCallConfirm ();
				m_nEQCallStep = 2;
				return true;

			case 2:
				SendAllEqData     ();
				m_nEQCallStep = 3;
				return true;

			case 3:
				Thread::SLEEP (50);
				SendEqCallEnded   ();
				m_nEQCallStep = 0;
				return true;
		}
	}

	m_nSendIdex = ((++m_nSendIdex) % 4);
	if (m_bIsRun) {
		switch(m_nSendIdex) {
			//For Deleting...
			case 2 : AccidentsHandler (SGIndex, 0);
			    break;
			//For Deleting...
			case 3 : AccidentsHandler (GJIndex, 1);
			    break;
		}
	}

	if (m_bIsRun && m_obj->isStart) {
		switch(m_nSendIdex) {
            case 0 : ForwardTellBWYX ();
                break;
			case 1 : ForwardTellBWYC ();
				break;
		}
	}
	return true;
}

bool C104::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
	if (len < 6)
		return true;

	iec104_apdu* papdu = (iec104_apdu*) pbuf;

	if (papdu->apci.startChr != 0x68)
		return true;

	Thread::SLEEP (50);
	wDebug() << "[Debug][" << (void*)this << "][Receive frame.]";

	m_obj->t3.start ();

	if (papdu->apci.length == 0x04)
	{
		switch (papdu->apci.NS) {
			case IEC104Obj::STARTDTACT : {
				SendStartConfirm();

				m_obj->init ();
				Thread::SLEEP (50);
				ForwardTellUnconfirm ();
				m_obj->isStart = true;
			} break;

            case IEC104Obj::STOPDTACT  : {
            	m_obj->isStart = false;
            	SendStopConfirm ();
            } break;

			case IEC104Obj::TESTFRACT  : {
				SendLinkTestConfirm();
			} break;

			case IEC104Obj::TESTFRCON  : {
			} break;

			case IEC104Obj::SUPERVISORY: {
				if (m_obj->VS == papdu->apci.NR)
					m_obj->cache.clear();
			} break;

			default  : {
			} break;
		}
		return true;
	}

	if ( !(papdu->apci.NS & 0x0001)) {
		m_obj->t2.start ();
		m_obj->PAddr = papdu->asduh.paddr;
		m_obj->VR += 2;

		if (m_obj->VS == papdu->apci.NR)
			m_obj->cache.clear();

		if (!(++(m_obj->wCount) % IEC104Obj::W_DEFAULT))
			SendMonitorFrame ();
	}

	switch (papdu->asduh.type) {
		case C_SC_NA_1:   	// 单点命令
		case C_DC_NA_1:   	// 双点命令
        case C_RC_NA_1:   	//
		case C_SE_NA_1:   	//
		case C_SE_NB_1:   	//
		case C_SE_NC_1: {   //
			TaskHandlerToLower (papdu);
        } break;

		case C_IC_NA_1: {	//
			if (papdu->asduh.cause != IecCause::ACT)
				break;

			m_nCallStep = m_obj->isStart ? 1 : 4;
		} break;

		case C_CI_NA_1: {	//
			if (papdu->asduh.cause != IecCause::ACT)
				break;

			if (papdu->nsq101.obj.frz == IEC_QCC::Freeze
			 && papdu->nsq101.obj.rqt == IEC_QCC::GroupAll)
			{
				m_nEQCallStep = m_obj->isStart ? 1 : 3;
			}
		} break;

		case C_CS_NA_1: {
			if (papdu->asduh.cause != IecCause::ACT)
				break;

			SendCompareTimeComfirm (papdu->nsq103.obj.ts);
		} break;

		default : {
			SendNAKComfirm (papdu, IecCause::UNKNOWNTYPEID);
		} break;
	}
	return true;
}

bool C104::IsSupportEngine (ST_INT engineType)
{
	return 0 == engineType; // EngineType == Fulling;
}

ST_VOID	C104::OnConnect(ST_INT port,ST_UINT64 portAddr)
{
	lDebug() << "[104 OnConnect] sockfd:" << port;
}

ST_VOID	C104::OnDisconnect(ST_INT port,ST_UINT64 portAddr)
{
	lDebug() << "[104 OnDisconnect] sockfd:" << port;
}

void C104::SendEx (iec104_apdu* papdu)
{
	if (!(papdu->apci.length)) return;

	Thread::SLEEP(50);
	m_bIsSendSuc = this->Send ((uint8_t*)papdu, papdu->apci.length + 2);
}

void C104::SendAndCache (iec104_apdu* papdu)
{
	m_obj->cache.add (papdu);
	this->SendEx (papdu);
	(m_obj->VS) += 2;
}

void C104::SendNotCache (iec104_apdu* papdu)
{
	this->SendEx (papdu);
	(m_obj->VS) += 2;
}

bool C104::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
	if (!this->GetDevice())
		return false;
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables) {
		wDebug() << "[Debug][" << (void*)this << "] No transfer tables.";
		return false;
	}

	if (trantables->GetCount() <= 0) {
		wDebug() << "[Debug][" << (void*)this << "] Transfer table count is 0.";
		return false;
	}
	int iter = 0;
	for (; iter < trantables->GetCount(); ++iter)
	{
		if (index == trantables->GetItem(iter)->typeId())
			break;
	}
	if ((table = trantables->GetItem(iter)) == NULL) {
		wDebug() << "[Debug][" << (void*)this << "] Not have this transfer table."
					 << "index:" << index;
		return false;
	}
	if ((list = table->m_pVarAddrs) == NULL) {
		wDebug() << "[Debug][" << (void*)this << "] Not have this list.";
		return false;
	}
	return true;
}

void C104::SendMonitorFrame ()
{
	ST_BYTE sendbuf[32] = {0};
	iec104_apci * papci = (iec104_apci*) sendbuf;

	papci->startChr	= IEC104Obj::STARTCHR;
	papci->length	= sizeof (iec104_apci) - 2;
	papci->NS  		= IEC104Obj::SUPERVISORY;
	papci->NR		= m_obj->VR;

	this->SendEx ((iec104_apdu*)sendbuf);

	wDebug() << "[Debug][" << (void*)this << "][Send monitor frame.]";
}

void C104::SendTestFrame ()
{
	ST_BYTE sendbuf[] = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
	this->SendEx ((iec104_apdu*)sendbuf);

	wDebug() << "[Debug][" << (void*)this << "][Send test frame.]";
}

void C104::SendStartConfirm ()
{
	ST_BYTE sendbuf[] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
	this->SendEx ((iec104_apdu*)sendbuf);

	wDebug() << "[Debug][" << (void*)this << "][Start confirm.]";
}

void C104::SendLinkTestConfirm ()
{
	ST_BYTE sendbuf[] = {0x68, 0x04, 0x83, 0x00, 0x00, 0x00};
	this->SendEx ((iec104_apdu*)sendbuf);

	wDebug() << "[Debug][" << (void*)this << "][Test confirm.]";
}

void C104::SendStopConfirm ()
{
	ST_BYTE sendbuf[] = {0x68, 0x04, 0x23, 0x00, 0x00, 0x00};
	this->SendEx ((iec104_apdu*)sendbuf);

	wDebug() << "[Debug][" << (void*)this << "][Stop confirm.]";
}

void C104::SendTotalCallConfirm ()
{
	ST_BYTE sendData[32] = {0};
	iec104_apdu * papdu 	= (iec104_apdu*) sendData;

	papdu->apci.startChr	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq100) - 2;
	papdu->asduh.type 		= C_IC_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.cause		= IecCause::ACTCON;
	papdu->asduh.paddr		= m_obj->PAddr;
	papdu->nsq100.obj.qoi 	= IEC_QOI::GroupAll;

	papdu->apci.NS			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Tocal call confirm.]";
}

void C104::SendEqCallConfirm ()
{
	ST_BYTE sendData[32] = {0};
	iec104_apdu* papdu 		= (iec104_apdu*) sendData;

	papdu->apci.startChr 	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq101) - 2;
	papdu->asduh.type 		= C_CI_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.cause 		= IecCause::ACTCON;
	papdu->asduh.paddr 		= m_obj->PAddr;
	papdu->nsq101.obj.rqt	= IEC_QCC::GroupAll;
	papdu->nsq101.obj.frz	= IEC_QCC::Freeze;

	papdu->apci.NS 			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Electric quantity call confirm.]";
}

void C104::SendCompareTimeComfirm (const CP56Time2a& ts)
{
	struct tm _tm;
	_tm.tm_sec  = ts.msec / 1000;
	_tm.tm_min  = ts.min;
	_tm.tm_hour = ts.hour;
	_tm.tm_mday = ts.mday;
	_tm.tm_mon  = ts.month - 1;
	_tm.tm_year = ts.year  + 100;

	time_t timep = mktime(&_tm);
	struct timeval tv;
	tv.tv_sec = timep;
	tv.tv_usec = 0;

	ST_BYTE sendData[32] = {0};
	iec104_apdu * papdu 	= (iec104_apdu*) sendData;

	papdu->apci.startChr	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq103) - 2;
	papdu->asduh.type 		= C_CS_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.paddr		= m_obj->PAddr;

	if(settimeofday (&tv, NULL) < 0)  {
		papdu->asduh.cause	= IecCause::ACTTERM;
		wDebug() << "[Debug][" << (void*)this << "] Set system datatime error!";
	}
	else {
		papdu->asduh.cause	= IecCause::ACTCON;
		wDebug() << "[Debug][" << (void*)this << "][compare the time confrim.]"
				 << ", year:"  << _tm.tm_year << ", month:" << _tm.tm_mon
				 << ", mday:"  << _tm.tm_mday << ", hour :" << _tm.tm_hour
				 << ", min :" << _tm.tm_min   << ", sec  :" << _tm.tm_sec;
	}
	memcpy (&(papdu->nsq103.obj.ts), &ts, sizeof (CP56Time2a));

	papdu->apci.NS			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);
}

void C104::SendNAKComfirm (const iec104_apdu* apdu, int32_t cause)
{
	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	memcpy (sendData, apdu, apdu->apci.length + 2);
	iec104_apdu* papdu  = (iec104_apdu*) sendData;

	papdu->apci.NS		= m_obj->VS;
	papdu->apci.NR		= m_obj->VR;
	papdu->asduh.cause	= (ST_BYTE)cause;
	papdu->asduh.pn 	= true;

	this->SendNotCache (papdu);
}

void C104::SendTotalCallEnded ()
{
	ST_BYTE sendData[32] = {0};
	iec104_apdu * papdu 	= (iec104_apdu*) sendData;

	papdu->apci.startChr	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq100) - 2;
	papdu->asduh.type 		= C_IC_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.cause		= IecCause::ACTTERM;
	papdu->asduh.paddr		= m_obj->PAddr;
	papdu->nsq100.obj.qoi 	= IEC_QOI::GroupAll;

	papdu->apci.NS			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Tocal call ended.]";
}

void C104::SendEqCallEnded ()
{
	ST_BYTE sendData[32] = {0};
	iec104_apdu* papdu 		= (iec104_apdu*) sendData;

	papdu->apci.startChr 	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq101) - 2;
	papdu->asduh.type 		= C_CI_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.cause 		= IecCause::ACTTERM;
	papdu->asduh.paddr 		= m_obj->PAddr;
	papdu->nsq101.obj.rqt	= IEC_QCC::GroupAll;
	papdu->nsq101.obj.frz	= IEC_QCC::Freeze;

	papdu->apci.NS 			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Electric quantity call Ended.]";
}

void C104::SendAllYXData ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;
	int32_t list_size = 0;

	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return ;

	list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

//this->GetCurLine()->m_TransformTableCount;
//this->GetCurLine()->m_TransformTables[ndb].type;
// 			pTranitems = this->GetCurLine()->m_TransformTables[ndb].items;
// 			nCount = this->GetCurLine()->m_TransformTables[ndb].count;
	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	int16_t frame_size  = sizeof (iec104_apci) + sizeof (iec104_data_unit_id);
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	(papdu->apci ).startChr	= IEC104Obj::STARTCHR;
	(papdu->asduh).type		= M_SP_NA_1;
	(papdu->asduh).sq 		= true;
	(papdu->asduh).cause 	= IecCause::INROGEN;
	(papdu->asduh).paddr 	= m_obj->PAddr;

	if (list_size > 0) {
		frame_size += 0x03;
	}

	int16_t cache_size = frame_size;
	unsigned char msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			(papdu->sq1).ioaddr16l = (iter + 0x0001) % 65536;
			(papdu->sq1).ioaddr8h  = (iter + 0x0001) / 65536;
		}
		//GetVariableValue(pTranitems[].area,pTranitems[].pointname,ffValue);
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;
		ST_BOOLEAN bValue = stvar.bVal;
		if (TRVar && TRVar->Coefficient == -1.0) bValue = !bValue;

		((papdu->sq1).obj[msgcount]).spi = bValue;
		++msgcount;
		m_hv.add (iter, stvar);

		frame_size += sizeof (iec_type1);
		if (msgcount > 0x77 || iter + 1 >= list_size)
		{
			(papdu->apci ).length	= frame_size - 2;
			(papdu->apci ).NS		= m_obj->VS;
			(papdu->apci ).NR		= m_obj->VR;
			(papdu->asduh).num      = msgcount;

			this->SendNotCache (papdu);

			wDebug() << "[Debug][" << (void*)this << "] TypeId: M_SP_NA_1, Cause: 20,"
					 << "Dit:" << iter << ", Total Dit:" << list_size << ", frame size:" << frame_size
					 << "[V(S) :" << m_obj->VS << ']';

			frame_size = cache_size;
			msgcount = 0;
		}
		Thread::SLEEP(1);
	}
}

void C104::SendAllYCData ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YCIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	int16_t frame_size  = sizeof (iec104_apci) + sizeof (iec104_data_unit_id);
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	(papdu->apci ).startChr	= IEC104Obj::STARTCHR;
	(papdu->asduh).type		= M_ME_NC_1;
	(papdu->asduh).sq 		= true;
	(papdu->asduh).cause 	= IecCause::INROGEN;
	(papdu->asduh).paddr 	= m_obj->PAddr;

	if (list_size > 0) {
		frame_size += 0x03;
	}

	int16_t cache_size = frame_size;
	unsigned char msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			(papdu->sq13).ioaddr16l = iter + 0x4001;
			(papdu->sq13).ioaddr8h	=(iter + 0x4001) / 63356;
			/*(papdu->sq13).ioaddr16l = iter + 0x0064;
			(papdu->sq13).ioaddr8h	=(iter + 0x0064) / 63356;*/
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_FLOAT varTemp = 0.0;
		if (TRVar) {
			if (TRVar->Coefficient == 0) TRVar->Coefficient = 1;
			varTemp = stvar.fVal * TRVar->Coefficient;
		}
		((papdu->sq13).obj[msgcount]).mv = varTemp;
		m_hv.add (iter + 0x4001, stvar);
		++msgcount;

		frame_size += sizeof (iec_type13);
		if (frame_size + (short)sizeof(iec_type13) > IEC104Obj::MaxFrameSize
								 || iter + 1 >= list_size)
		{
			(papdu->apci ).length	= frame_size - 2;
			(papdu->apci ).NS		= m_obj->VS;
			(papdu->apci ).NR		= m_obj->VR;
			(papdu->asduh).num      = msgcount;

			this->SendNotCache (papdu);

			wDebug() << "[Debug][" << (void*)this << "] TypeId: M_ME_NC_1, Cause: 20,"
					 << "Dit:" << iter << ", frame size:" << frame_size
					 << "[V(S) :" << (m_obj->VS) << ']';

			frame_size   = cache_size;
			msgcount     = 0;
		}
		Thread::SLEEP(1);
	}
}

void C104::SendAllEqData ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YMIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	int16_t frame_size  = sizeof (iec104_apci) + sizeof (iec104_data_unit_id);
	iec104_apdu* papdu  = (iec104_apdu*) sendData;

	(papdu->apci ).startChr	= IEC104Obj::STARTCHR;
	(papdu->asduh).type		= M_IT_NA_1;
	(papdu->asduh).sq 		= true;
	(papdu->asduh).cause 	= IecCause::INROGEN;
	(papdu->asduh).paddr 	= m_obj->PAddr;

	if (list_size > 0) {
		frame_size += 0x03;
	}

	int16_t cache_size = frame_size;
	unsigned char msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		if (frame_size == cache_size) {
			(papdu->sq15).ioaddr16l = (iter + 0x6401) % 65536;
			(papdu->sq15).ioaddr8h  = (iter + 0x6401) / 65536;
		}
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		int64_t llTemp = 0;

		if (TRVar) {
			if (TRVar->Coefficient == 0.0)
				TRVar->Coefficient = 1;
			if (stvar.vt == VALType_Double)
				llTemp = (int64_t)(stvar.dVal * TRVar->Coefficient);
			else
				llTemp = (int64_t)(stvar.fVal * TRVar->Coefficient);
		}

		((papdu->sq15).obj[msgcount]).mv = static_cast<int32_t>(llTemp);
		// m_hv.add (iter + 0x6401, stvar);
		++msgcount;

		frame_size += sizeof (iec_type15);
		if (frame_size + (short)sizeof(iec_type15) > IEC104Obj::MaxFrameSize
								 || iter + 1 >= list_size)
		{
			(papdu->apci ).length	= frame_size - 2;
			(papdu->apci ).NS		= m_obj->VS;
			(papdu->apci ).NR		= m_obj->VR;
			(papdu->asduh).num      = msgcount;

			this->SendNotCache (papdu);

			wDebug() << "[Debug][" << (void*)this << "] TypeId: M_IT_NA_1, Cause: 20,"
					 << "Dit:" << iter << ", frame size:" << frame_size
					 << "[V(S) :" << (m_obj->VS) << ']';

			frame_size   = cache_size;
			msgcount     = 0;
		}
		Thread::SLEEP(1);
	}
}

void C104::ForwardTellBWYX ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	int16_t frame_size  = sizeof (iec104_apci) + sizeof (iec104_data_unit_id);
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	papdu->apci.startChr	= IEC104Obj::STARTCHR;
	papdu->asduh.type		= M_SP_NA_1;
	papdu->asduh.sq 		= false;
	papdu->asduh.cause 		= IecCause::SPONTANEOUS;
	papdu->asduh.paddr 		= m_obj->PAddr;

	int16_t cache_size = frame_size;
	unsigned char msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_VARIANT * tempvar = m_hv.find (iter);
		if (tempvar && tempvar->bVal != stvar.bVal) {
			ST_BOOLEAN bValue = stvar.bVal;
			if (TRVar->Coefficient == -1.0) bValue = !bValue;
			(papdu->nsq1[msgcount]).obj.spi   = bValue;
			(papdu->nsq1[msgcount]).ioaddr8h  = (iter + 0x0001) / 65536;
			(papdu->nsq1[msgcount]).ioaddr16l = (iter + 0x0001) % 65536;

			m_hv.add (iter, stvar);
			++msgcount;
			frame_size += sizeof (papdu->nsq1[0]);

			wDebug() << "[Debug][" << (void*)this << "cur var:" << stvar.ulVal << "src var:" << tempvar->ulVal
					 << ", " << *duaddr << "nR:" << nR;
		}

		if (msgcount && (msgcount > 0x77 || iter + 1 >= list_size))
		{
			papdu->apci.length	= frame_size - 2;
			papdu->apci.NS		= m_obj->VS;
			papdu->apci.NR		= m_obj->VR;
			papdu->asduh.num    = msgcount;

			this->SendAndCache (papdu);

			wDebug() << "[Debug][" << (void*)this << "] TypeId: M_SP_NA_1, Cause: 3,"
					 << "[V(S) :" << (papdu->apci.NS) << "]["
					 << CDebug::bytesToHexString (sendData, frame_size) << ']';

			frame_size   = cache_size;
			msgcount     = 0;
		}
	}
}

void C104::ForwardTellBWYC ()
{
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YCIndex, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_BYTE sendData[IEC104Obj::MaxFrameSize] = {0};
	int16_t frame_size  = sizeof (iec104_apci) + sizeof (iec104_data_unit_id);
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	papdu->apci.startChr	= IEC104Obj::STARTCHR;
	papdu->asduh.type		= M_ME_NC_1;
	papdu->asduh.sq 		= false;
	papdu->asduh.cause 		= IecCause::SPONTANEOUS;
	papdu->asduh.paddr 		= m_obj->PAddr;

	int16_t cache_size = frame_size;
	unsigned char msgcount = 0;
	for (int32_t iter = 0; iter < list_size; ++iter)
	{
		ST_DUADDR  *duaddr = tablelist->GetItem(iter);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		ST_VARIANT stvar;
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_VARIANT * oldstvar = m_hv.find (iter + 0x4001);
		if (oldstvar && fabs (oldstvar->fVal - stvar.fVal) > 0.01) {
			if (TRVar->Coefficient == 0)
				TRVar->Coefficient = 1;

			ST_FLOAT varTemp = stvar.fVal * TRVar->Coefficient;
			papdu->nsq13[msgcount].obj.mv = varTemp;
			papdu->nsq13[msgcount].ioaddr8h  = (iter + 0x4001) / 65536;
			papdu->nsq13[msgcount].ioaddr16l = (iter + 0x4001) % 65536;

			m_hv.add (iter + 0x4001, stvar);
			++msgcount;
			frame_size += sizeof (papdu->nsq13[0]);
		}

		if (msgcount && (frame_size + (short)sizeof(papdu->nsq13[0]) > IEC104Obj::MaxFrameSize
								|| iter + 1 >= list_size))
		{
			papdu->apci.length	= frame_size - 2;
			papdu->apci.NS		= m_obj->VS;
			papdu->apci.NR		= m_obj->VR;
			papdu->asduh.num    = msgcount;

			this->SendNotCache (papdu);

			wDebug() << "[Debug][" << (void*)this << "] TypeId: M_ME_NC_1, Cause: 3,"
					 << "[V(S) :" << (papdu->apci.NS) << "]["
					 << CDebug::bytesToHexString (sendData, frame_size) << ']';

			frame_size   = cache_size;
			msgcount     = 0;
		}
	}
}

void C104::ForwardTellSOE (ProtocolTask& task)
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
		return;

	ST_BYTE sendData[32] = {0};
	iec104_apdu * papdu = (iec104_apdu*) sendData;

	papdu->apci.startChr		= IEC104Obj::STARTCHR;
	papdu->apci.length			= sizeof (papdu->apci) + sizeof (papdu->asduh)
								+ sizeof (papdu->nsq30[0]) - 2;
	papdu->asduh.type 			= M_SP_TB_1;
	papdu->asduh.num 			= 0x01;
	papdu->asduh.cause			= IecCause::SPONTANEOUS;
	papdu->asduh.paddr			= m_obj->PAddr;

	papdu->nsq30[0].ioaddr8h	= (iter + 0x0001) / 65536;
	papdu->nsq30[0].ioaddr16l	= (iter + 0x0001) % 65536;
	papdu->nsq30[0].obj.spi 	= task.taskValue;
	papdu->nsq30[0].obj.ts.msec	= task.taskParam[7] + task.taskParam[8] * 256
								+ task.taskParam[6] * 1000; //msecond
	papdu->nsq30[0].obj.ts.min	= task.taskParam[5] & 0x3F;
	papdu->nsq30[0].obj.ts.hour	= task.taskParam[4] & 0x1F;
	papdu->nsq30[0].obj.ts.mday	= task.taskParam[3] & 0x1F; //day
	papdu->nsq30[0].obj.ts.month= task.taskParam[2] & 0x0F; //month
	papdu->nsq30[0].obj.ts.year	=(task.taskParam[0] + task.taskParam[1] * 256 - 2000) & 0x7F; //year

	papdu->apci.NS				= m_obj->VS;
	papdu->apci.NR				= m_obj->VR;
	this->SendAndCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Forward tell SOE.]";

	memset (&task, 0, sizeof (task));
}

void C104::ForwardTellUnconfirm ()
{
	int size = m_obj->cache.size();
	for (int i = 0; i < size; ++i)
	{
		iec104_apdu* papdu = m_obj->cache.at(i);
		papdu->apci.NR = m_obj->VR;
		papdu->apci.NS = m_obj->VS;

		this->SendNotCache (papdu);
	}
}

bool C104::TransC_SC_NA_1ToLower (const iec104_apdu* papdu)
{
	wDebug() << "[Debug][" << (void*)this << "] Check C_SC_NA_1 Dit.";

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
		return false;

	int32_t list_size = tablelist->GetCount();

	int32_t iter = (papdu->nsq45.ioaddr8h * 65536 + papdu->nsq45.ioaddr16l) - 0x6001;
	if (list_size <= 0 || list_size <= iter || iter < 0) {
		wDebug() << "[Debug][" << (void*)this << "] UNKNOWNINFOADDR: " << iter;
		SendNAKComfirm (papdu, IecCause::UNKNOWNINFOADDR);
		return false;
	}

	ST_DUADDR* pda = tablelist->GetItem(iter);

	ProtocolTask *task 		= new ProtocolTask;
	task->isTransfer 		= true;
	task->transChannelId 	= -1;
	task->transDeviceId 	= pda->device;
	task->taskAddr 			= pda->addr;
	task->taskValue 		= papdu->nsq45.obj.scs;

	switch (papdu->asduh.cause) {
		case IecCause::ACT  : {
			task->taskCmdCode 	= (papdu->nsq45.obj.se ? 0x00 : 0x01 );

		} break;

		case IecCause::DEACT: {
			task->taskCmdCode 	= 0x02;
		} break;
	}
	strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));

	Transfer(task);
	delete task;

	wDebug() << "[Debug][" << (void*)this << "][Transfer C_SC_NA_1 order.]";

	return true;
}

void C104::TransC_SC_NA_1ToUpper (const ProtocolTask& task)
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

	wDebug() << "[Debug][" << (void*)this << "] Part C_SC_NA_1 task.";

	ST_BYTE sendData[32] = {0};
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	papdu->apci.startChr 	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq45) - 2;
	papdu->asduh.type 		= C_SC_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.paddr 		= m_obj->PAddr;
	papdu->nsq45.ioaddr8h	= iter / 65536;
	papdu->nsq45.ioaddr16l  = iter % 65536;
	papdu->nsq45.obj.scs	= task.taskValue;

	switch (task.taskCmdCode) {
		case 0x00 : papdu->nsq45.obj.se = 0x1;
					papdu->asduh.cause  = IecCause::ACTCON;
					break;
		case 0x01 : papdu->nsq45.obj.se = 0x0;
					papdu->asduh.cause  = IecCause::ACTCON;
					break;
		case 0x02 : papdu->nsq45.obj.se = 0x0;
					papdu->asduh.cause  = IecCause::DEACTCON;
					break;
	}

	papdu->apci.NS 			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Forward tell C_SC_NA_1 result.]"
			 << CDebug::bytesToHexString ((uint8_t*)papdu, papdu->apci.length + 2);
}

bool C104::TransC_DC_NA_1ToLower (const iec104_apdu* papdu)
{
	if (papdu->nsq46.obj.dcs == 0x00 || papdu->nsq46.obj.dcs == 0x03) {
		return false;
	}

	wDebug() << "[Debug][" << (void*)this << "] Check C_DC_NA_1 Dit.";

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (YKIndex, trantable, tablelist))
		return false;

	int32_t list_size = tablelist->GetCount();

	int32_t iter = (papdu->nsq46.ioaddr8h * 65536 + papdu->nsq46.ioaddr16l) - 0x6001;

	if (list_size <= 0 || list_size <= iter || iter < 0) {
		wDebug() << "[Debug][" << (void*)this << "] UNKNOWNINFOADDR: " << iter;
		SendNAKComfirm (papdu, IecCause::UNKNOWNINFOADDR);
		return false;
	}
	ST_DUADDR* pda = tablelist->GetItem(iter);

	wDebug() << "[Debug][" << (void*)this << "] Part C_DC_NA_1 task.";

	ProtocolTask *task 		= new ProtocolTask;
	task->isTransfer 		= true;
	task->transChannelId 	= -1;
	task->transDeviceId 	= pda->device;
	task->taskAddr 			= pda->addr;
	task->taskValue 		= (papdu->nsq46.obj.dcs == 0x02 ? 0x01 : 0x00);
	switch (papdu->asduh.cause) {
		case IecCause::ACT  : {
			task->taskCmdCode 	= (papdu->nsq46.obj.se ? 0x00 : 0x01 );
		} break;

		case IecCause::DEACT: {
			task->taskCmdCode 	= 0x02;
		} break;
	}
	strncpy(task->taskCmd, "devicecontrol", sizeof (task->taskCmd));

	Transfer(task);
	delete task;

	wDebug() << "[Debug][" << (void*)this << "][Transfer C_DC_NA_1 order.]";

	return true;
}

void C104::TransC_DC_NA_1ToUpper (const ProtocolTask& task)
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
	iec104_apdu* papdu = (iec104_apdu*) sendData;

	papdu->apci.startChr 	= IEC104Obj::STARTCHR;
	papdu->apci.length		= sizeof (papdu->apci) + sizeof (papdu->asduh)
							+ sizeof (papdu->nsq46) - 2;
	papdu->asduh.type 		= C_DC_NA_1;
	papdu->asduh.num 		= 0x01;
	papdu->asduh.paddr 		= m_obj->PAddr;
	papdu->nsq46.ioaddr8h	= iter / 65536;
	papdu->nsq46.ioaddr16l  = iter % 65536;
	papdu->nsq46.obj.dcs	= (task.taskValue == 0x1 ? 0x2 : 0x1);

	switch (task.taskCmdCode) {
		case 0x00 : papdu->nsq46.obj.se = 0x1;
					papdu->asduh.cause  = IecCause::ACTCON;
					break;
		case 0x01 : papdu->nsq46.obj.se = 0x0;
					papdu->asduh.cause  = IecCause::ACTCON;
					break;
		case 0x02 : papdu->nsq46.obj.se = 0x0;
					papdu->asduh.cause  = IecCause::DEACTCON;
					break;
	}
	papdu->apci.NS 			= m_obj->VS;
	papdu->apci.NR			= m_obj->VR;
	this->SendNotCache (papdu);

	wDebug() << "[Debug][" << (void*)this << "][Forward tell C_DC_NA_1 result.]" << task;
}

void C104::TaskHandlerToLower (const iec104_apdu* papdu)
{
	if (m_iecTaskType != Undefined)
		return;


	if (!m_pPorts->CanRemoteCtrl()) {
		SendNAKComfirm(papdu, IecCause::ACTTERM);
		return;
	}


	taskexpire.start ();

	wDebug() << "[Debug][" << (void*)this << "] Task transfer handler for upper. type:";

	m_iecTaskType = papdu->asduh.type;

	bool result = false;
	switch (papdu->asduh.type) {
		case C_SC_NA_1: {   // 单点命令
			result = TransC_SC_NA_1ToLower (papdu);
		} break;
		case C_DC_NA_1: {   // 双点命令
			result = TransC_DC_NA_1ToLower (papdu);
		} break;
		case C_RC_NA_1: {   //
		} break;
		case C_SE_NA_1: {   //
		} break;
		case C_SE_NB_1: {   //
		} break;
		case C_SE_NC_1: {   //
		} break;
	}
	if (!result) m_iecTaskType = Undefined;
}

void C104::TaskHandlerToUpper (ProtocolTask& task)
{
	wDebug() << "[Debug][" << (void*)this << "] Task transfer handler for lower. type:"
			 << m_iecTaskType;

	switch (m_iecTaskType) {
		case C_SC_NA_1: {   // 单点命令
		    TransC_SC_NA_1ToUpper (task);
		} break;
		case C_DC_NA_1: {   // 双点命令
		    TransC_DC_NA_1ToUpper (task);
		} break;
		case C_RC_NA_1: {   //
		} break;
		case C_SE_NA_1: {   //
		} break;
		case C_SE_NB_1: {   //
		} break;
		case C_SE_NC_1: {   //
		} break;
	}

	memset (&task, 0, sizeof (task));
	m_iecTaskType = Undefined;
}

//For Deleting...
void C104::AccidentsHandler (int32_t tableIndex, int32_t addr)
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

        if (bValue == true) {
			wDebug() << "[Debug][" << (void*)this << "cur var:" << stvar.ulVal
					 <<  ", " << *duaddr << "nR:" << nR;
            break;
        }
    }

    this->UpdateValue (addr, (sg_iter < sglist_size ? true : false));

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return;

    int32_t yxlist_size = tablelist->GetCount();
    int32_t iter = 0;
    for (; iter < yxlist_size; ++iter) {
        ST_DUADDR * pda = tablelist->GetItem(iter);
        if (addr == pda->addr
            && this->GetDevice()->GetDeviceInfo()->DeviceId == pda->device)
            break;
    }
    if (iter == yxlist_size)
        return;

    ST_VARIANT * tempval = m_hv.find (iter);
    if (!tempval) return;
    if ( tempval->bVal == stvar.bVal)
        return;

    wDebug() << "[Debug][" << (void*)this
    	     << "][SGTotal change.] cur var:" << stvar.ulVal << "src var:" << tempval->ulVal
    		 << ", size:"  << yxlist_size << ", "  << *(tablelist->GetItem(iter));

    m_hv.add (iter, stvar);

    ST_BYTE sendData[32] = {0};
    iec104_apdu * papdu = (iec104_apdu*) sendData;

    papdu->apci.startChr        = IEC104Obj::STARTCHR;
    papdu->apci.length          = sizeof (papdu->apci) + sizeof (papdu->asduh)
                                + sizeof (papdu->nsq30[0]) - 2;
    papdu->asduh.type           = M_SP_TB_1;
    papdu->asduh.num            = 0x01;
    papdu->asduh.cause          = IecCause::SPONTANEOUS;
    papdu->asduh.paddr          = m_obj->PAddr;

    struct tm tm_now;
    DateTime::localtime (time(0), tm_now);

    papdu->nsq30[0].ioaddr8h	=(iter + 0x0001) / 65536;
    papdu->nsq30[0].ioaddr16l   =(iter + 0x0001) % 65536;
    papdu->nsq30[0].obj.spi     = bValue;
    papdu->nsq30[0].obj.ts.msec = tm_now.tm_sec  * 999 ;   //second
    papdu->nsq30[0].obj.ts.min  = tm_now.tm_min  & 0x3F;
    papdu->nsq30[0].obj.ts.hour = tm_now.tm_hour & 0x1F;
    papdu->nsq30[0].obj.ts.mday = tm_now.tm_mday & 0x1F;   //day
    papdu->nsq30[0].obj.ts.wday = tm_now.tm_wday & 0x07;
    papdu->nsq30[0].obj.ts.month= tm_now.tm_mon  + 1;      //month
    papdu->nsq30[0].obj.ts.year = tm_now.tm_year - 100;    //year

    papdu->apci.NS              = m_obj->VS;
    papdu->apci.NR              = m_obj->VR;
    this->SendAndCache (papdu);

    wDebug() << "[Debug][" << (void*)this << "][SG SOE Forward tell.]";

    Thread::SLEEP (20);
}
