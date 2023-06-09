
#include "kdp8200.h"
#include "Device.h"
#include "datetime.h"

inline uint8_t get_data_size (const uint8_t* data)
{
	if (*data == 0x68) return data[1] + 6;
	return 0;
}

inline uint8_t get_check_sum (const uint8_t* data)
{
	int16_t  len  = 0;
	const uint8_t* iter = 0;
	if (*data == 0x68) {
		iter = data + 4; len = data[1];
	}
	uint8_t sum = 0;
	while (len-- > 0)
		sum += *iter++;
	return sum;
}

inline bool is_cs_error (const uint8_t* data)
{
	if (data[1] != data[2] && data[0] != data[3])
		return true;

	return data[get_data_size(data) - 2] != get_check_sum(data);
}

enum ORDER_CODE
{
	ORDER_READ_CLOCK    = 0x05,
	ORDER_TOCAL_CALL    = 0x07,
	ORDER_YK_SELECT     = 0x08,
	ORDER_YK_EXEC       = 0x09,
	ORDER_YK_CANCEL     = 0x0A,
	ORDER_CALL_SOE      = 0x0B,
	ORDER_CALL_SETTING  = 0x0C,
	ORDER_CALL_EVENT    = 0x0D,
	ORDER_CALL_PLATE    = 0x0E,
	ORDER_WRITE_SETTING = 0x1C,
	ORDER_WRITE_PLATE   = 0x1E
};

CKDP8200* CreateInstace()
{
	return new CKDP8200();
}

CKDP8200::CKDP8200 ():
m_bIsSendSuc(false),
m_bTask(false),
m_sendindex(0),
m_is_syc_clock(false),
m_lasttime(time(0))
{
}

CKDP8200::~CKDP8200()
{
}

void CKDP8200::Init  ()
{
}

void CKDP8200::Uninit()
{
}

void CKDP8200::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	do {
		if(! this->IsOpened()) {
			break;
		}
		if(! this->GetCurPort ()) {
			break;
		}
		int32_t	len = this->GetCurPort()->PickBytes(pbuf, 6, 2000);
		if(len < 6) {
			this->GetCurPort()->Clear();
			this->ShowMessage ("Insufficient data length.");
			break;
		}
		int32_t star = 0;
		for(; star < len; ++star) {
			if(pbuf[star] == 0x68)
				break;
		}
/*		if(star == len)
		{
			this->GetCurPort()->Clear();
			break;
		}*/
		if(star > 0) {
			this->GetCurPort()->ReadBytes(pbuf, star);
		}
		len = this->GetCurPort()->PickBytes(pbuf, 6, 2000);

		int32_t ndatalen = get_data_size(pbuf);
		len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
		if(len != ndatalen) {
			this->GetCurPort()->Clear();
			this->ShowMessage ("Insufficient data length.");
			break;
		}
		if(is_cs_error(pbuf)) {
			this->GetCurPort()->Clear();
			this->ShowMessage ("Check error.");
			break;
		}
		readed = len;
	} while (0);
}

ST_BOOLEAN CKDP8200::OnSend ()
{
	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			TaskHandlerToLower (m_curTask);
			m_bTask = true;
			return true;
		}
	}

	time_t tnow = time(0);
	double interval = difftime (tnow, m_lasttime);
	if (tnow < m_lasttime || (m_is_syc_clock && interval > 30*60)) {

		m_lasttime = time(0);
	}

	switch (m_sendindex++ % 3) {
		case 0: SendCall (ORDER_CALL_SOE)  ; break;
		case 1: SendCall (ORDER_CALL_EVENT); break;
		case 2: SendCall (ORDER_TOCAL_CALL); break;
	}
	return true;
}

void CKDP8200::SendEx (uint8_t* data)
{
	int32_t len   = get_data_size (data);
	if (!len) return;
	data[len - 2] = get_check_sum (data);
	data[len - 1] = 0x16;
	m_bIsSendSuc  = this->Send(data, len);
}

ST_BOOLEAN CKDP8200::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	if (m_bTask) {
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
	}

	switch (pbuf[4]) {
		case ORDER_READ_CLOCK: {
		} break;
		case ORDER_TOCAL_CALL: {
			AnalyzerForYXYC (pbuf);
		} break;
		case ORDER_YK_SELECT:
		case ORDER_YK_EXEC  :
		case ORDER_YK_CANCEL: {
		} break;
		case ORDER_CALL_EVENT:
		case ORDER_CALL_SOE: {
			AnalyzerForSOE  (pbuf);
		} break;
		case ORDER_CALL_SETTING: {
		} break;
		case ORDER_CALL_PLATE: {
		} break;
		case ORDER_WRITE_PLATE: {
		} break;
		case ORDER_WRITE_SETTING: {
		} break;
	}
	return true;
}

ST_BOOLEAN CKDP8200::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void CKDP8200::SendCall (ST_BYTE code)
{
	ST_BYTE datas[16] = {0};
	datas[0x00] = 0x68;
	datas[0x01] = 0x02;
	datas[0x02] = 0x02;
	datas[0x03] = 0x68;
	datas[0x04] = code;
	datas[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

	this->SendEx (datas);
}

#pragma pack(push,1)	// 紧凑对齐

struct SOE_STRUCT {
	uint8_t  num;
	uint8_t  attr;
	uint8_t  value;
	uint16_t msec;
	uint8_t  min;
	uint8_t  hour;
	uint8_t  day;
	uint8_t  month;
	uint8_t  year;
};

#pragma pack(pop)		// 恢复默认内存对齐

void CKDP8200::AnalyzerForSOE (const ST_BYTE* data)
{
	const ST_BYTE *dataptr = data + 6;
	ST_BYTE len = *dataptr++;
	const DeviceInfo& devinfo = *this->GetDevice()->GetDeviceInfo();
	for (ST_BYTE index = 0; index < len; ++index, dataptr += sizeof (SOE_STRUCT))
	{
		const SOE_STRUCT* stsoe = (const SOE_STRUCT*)dataptr;
		ProtocolTask task;

		Strcpy (task.taskCmd, "SOE");
		task.isTransfer  = true;
		task.taskCmdCode = 0;
		task.taskValue   = stsoe->value;
		task.taskAddr1   = devinfo.DeviceId;
		task.ignoreBack  = 1;
		task.taskTime    = 1000;
		task.taskAddr    = stsoe->attr * 10000 + stsoe->num;

		task.taskParamLen    = 14;
		task.taskParam[0x00] = (stsoe->year + 2000) % 256;
		task.taskParam[0x01] = (stsoe->year + 2000) / 256;
		task.taskParam[0x02] = (stsoe->month);
		task.taskParam[0x03] = (stsoe->day);
		task.taskParam[0x04] = (stsoe->hour);
		task.taskParam[0x05] = (stsoe->min);
		task.taskParam[0x06] = (stsoe->msec / 1000);
		task.taskParam[0x07] = (stsoe->msec % 1000) % 256;
		task.taskParam[0x08] = (stsoe->msec % 1000) / 256;
		task.taskParam[0x09] = (stsoe->value);

		task.taskParam[0x0A] = task.taskAddr  % 256;
		task.taskParam[0x0B] = task.taskAddr  / 256;
		task.taskParam[0x0C] = task.taskAddr1 % 256;
		task.taskParam[0x0D] = task.taskAddr1 / 256;

		Transfer(&task);
	}
}

void CKDP8200::AnalyzerForYXYC(const ST_BYTE* data)
{
	const ST_BYTE *dataptr = data + 6;
	ST_BYTE len = *dataptr++;
	for (ST_BYTE index = 0; index < len; ++index)
	{
		float value = 0;
		memcpy (&value, dataptr, sizeof(value));
		dataptr += sizeof (value);
		this->UpdateValue (index + 30000, value);
	}

	len = *dataptr * 8;
	for (ST_BYTE index = 0; index < len; ++index)
	{
		if ((index % 8) == 0)
			++dataptr;
		ST_BYTE value = (bool)(*dataptr & (0x01 << (index % 8)));
		this->UpdateValue (index, value);
	}
}

void CKDP8200::TaskHandlerToLower (const ProtocolTask& task)
{
	ST_BYTE datas[16] = {0};
	datas[0x00] = 0x68;
	datas[0x01] = 0x04;
	datas[0x02] = 0x04;
	datas[0x03] = 0x68;

	datas[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	datas[0x06] = task.taskAddr;
	datas[0x07] = task.taskValue;

	switch (task.taskCmdCode) {
		case 0: datas[0x04] = 0x08;
			break;
		case 1: datas[0x04] = 0x09;
			break;
		case 2: datas[0x04] = 0x0A;
			break;
	}
	this->SendEx (datas);
}
