
#include "sytec200d.h"
#include "datetime.h"
#include "Channel.h"

#pragma pack(push,1)	// 紧凑对齐

struct YMType {
    ST_INT32 mv;
    unsigned char sq :5;
    unsigned char cy :1; // carry/no carry
    unsigned char ca :1; // Counter was adjusted/Counter was not adjusted
    unsigned char iv :1;
};

struct CtrlCode {
    unsigned char fc  :4;
    unsigned char fd  :1; // FCV/DFC
    unsigned char fa  :1; // FCB/ACD
    unsigned char prm :1;
    unsigned char dir :1;
};

#pragma pack(pop)		// 恢复默认内存对齐

inline uint8_t get_data_size (const uint8_t* data)
{
	if (*data == 0x10) return 5;
	if (*data == 0x68) return data[1] +6;
	return 0;
}

inline uint8_t get_check_sum (const uint8_t* data)
{
	int16_t  len  = 0;
	const uint8_t* iter = 0;
	if (*data == 0x10) {
		iter = data + 1; len = 2;
	}
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
	if (*data == 0x10 && data[4] != 0x16)
		return true;
	if (*data == 0x68 && data[1] != data[2] && data[0] != data[3])
		return true;

	return data[get_data_size(data) - 2] != get_check_sum(data);
}

SYTEC200D* CreateInstace()
{
	return new SYTEC200D();
}


SYTEC200D::SYTEC200D()
{
}

SYTEC200D::~SYTEC200D()
{
}

void SYTEC200D::Init()
{
	m_bIsSendSuc = false;
	m_has1stData = false;
	m_bTask      = false;
	m_fcb_flag   = false;

	m_totalcallcount = 0;
	m_lasttime = time(0);
	m_curTask.Clear();
}

void SYTEC200D::Uninit()
{}

void SYTEC200D::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	if(!this->GetCurPort() || !this->GetDevice())
		return ;

	ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);

	ST_INT	len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	if(len < 5) {
		ShowMessage ("1:Insufficient data length");
		return ;
	}

	ST_BYTE frametype = 0;
	ST_INT star = 0;
	for(; star < len; ++star) {
		if((pbuf[star] == 0x10) || (pbuf[star] == 0x68))
		{
			frametype = pbuf[star];
			break;
		}
	}
	if(star == len) {
		this->ShowRecvFrame(pbuf, len);
		ShowMessage ("Garbled code, clear buffer");
		this->GetCurPort()->Clear();
		return ;
	}
	if(star > 0) {
		ShowMessage ("Has Garbled code");
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	// 找到头了，进行完成 头判断
	if(frametype == 0x10)
	{
		len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
		if(len < 5) {
			ShowMessage ("2:Insufficient data length");
			this->GetCurPort()->Clear();
			return ;
		}
		if(pbuf[0] == 0x10 && (pbuf[4] == 0x16))
		{
			readed = this->GetCurPort()->ReadBytes(pbuf, 5);
			if (get_check_sum(pbuf) != pbuf[3]) {
				ShowMessage ("Check error!");
				readed = 0;
			}
			return ;
		}
		else
		{
			ST_INT ierr;
			for(ierr = 1; ierr < 5; ++ierr)
			{
				if(pbuf[ierr] == 0x10 || pbuf[ierr] == 0x68)
					break;
			}
			this->GetCurPort()->ReadBytes(pbuf, ierr);
		}
	}
	else if(frametype == 0x68)
	{
		len = this->GetCurPort()->PickBytes(pbuf, 10, interval);
		if(len < 10) {
			ShowMessage ("3:Insufficient data length");
	        this->GetCurPort()->Clear();
	        return ;
		}
		ST_INT naddr = this->GetDevice()->GetDeviceInfo()->Address;
		if(((pbuf[0] == 0x68) && (pbuf[3] == 0x68) && (pbuf[1] == pbuf[2]))&& (pbuf[5] == (ST_BYTE)naddr))
		{
			//变长帧帧头判断通过
			ST_INT datalen = pbuf[1]+6;
			len = this->GetCurPort()->PickBytes(pbuf, datalen, interval);
			if(len < datalen) {
				ShowMessage ("4:Insufficient data length");
				this->GetCurPort()->Clear();
				return ;
			}
			readed = this->GetCurPort()->ReadBytes(pbuf, datalen);
			if(readed != datalen || pbuf[readed-1] != 0x16)
			{
	            readed = 0;
	            ShowMessage ("5:Insufficient data length");
                this->GetCurPort()->Clear();
				return;
			}
			if (get_check_sum(pbuf) != pbuf[readed-2])
			{
				readed = 0;
				ShowMessage ("Check error!");
				this->GetCurPort()->Clear();
			}
		}
		else
		{
			ShowMessage ("Sync header error!");
			this->GetCurPort()->Clear();
		}
	}
}

ST_BOOLEAN SYTEC200D::OnSend()
{
	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0) {
				YKSelect(m_curTask);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 1) {
				YKExecut(m_curTask);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 2) {
				YKCancel(m_curTask);
				m_bTask = true;
				return true;
			}
		}
	}

	if (m_has1stData) {
		m_has1stData = false;
		Call1stData ();
		return true;
	}

	if (m_totalcallcount++ == 0) {
		CallTotal();
		return true;
	}

	if (m_totalcallcount   == 3) {
		CallYMData();
		return true;
	}

    time_t tnow = time(0);
    double interval = difftime(tnow, m_lasttime);
	if(tnow < m_lasttime || interval > 30*60)
	{
		SetTime();
		time(&m_lasttime);
		Thread::SLEEP(20);
		return true;
	}

	Call2ndData ();
	return true;
}

ST_BOOLEAN SYTEC200D::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	if (pbuf[4] & 0x20) {
		m_has1stData = true;
		// this->Continue ();
	}

	if (len <= 5) {
		return true;
	}

	if (m_bTask) {
		if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
			m_curTask.isTransfer = 1;
			Transfer(&m_curTask);
			memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		return false;
	}

	switch (pbuf[6]) {
		case 0x09: {
			AnalysisFor2ndData(pbuf);
		} break;
		case 0x2C: {
			AnalysisForYXData (pbuf);
		} break;
		case 0x01: {
			AnalysisForSOE    (pbuf);
		} break;
		case 0x24: {
			AnalysisForYMData (pbuf);
		} break;
	}
	return true;
}

ST_BOOLEAN SYTEC200D::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void SYTEC200D::SendEx (ST_BYTE * data)
{
	int32_t len   = get_data_size (data);
	if (!len) return;
	data[len - 2] = get_check_sum (data);
	data[len - 1] = 0x16;
	m_bIsSendSuc  = this->Send(data, len);
}

ST_BYTE SYTEC200D::GetCtrlCode (ST_BYTE code, bool fcv_flag)
{
	CtrlCode cc;
	cc.dir = 0;
	cc.prm = 1;
	cc.fc  = code;
	cc.fd  = fcv_flag;

	if (fcv_flag) {
		cc.fa = m_fcb_flag;
		m_fcb_flag = !m_fcb_flag;
	}
	else
		cc.fa = 0;

	return *((ST_BYTE*)&cc);
}

void SYTEC200D::Call1stData ()
{
	ST_BYTE data[32] = {0};
	data[0] = 0x10;
	data[1] = GetCtrlCode (0x0A, true);
	data[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

	this->SendEx (data);
}

void SYTEC200D::Call2ndData ()
{
	ST_BYTE data[32] = {0};
	data[0] = 0x10;
	data[1] = GetCtrlCode (0x0B, true);
	data[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

	this->SendEx (data);
}

void SYTEC200D::CallTotal ()
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x09;
	data[0x02] = 0x09;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x07;
	data[0x07] = 0x81;
	data[0x08] = 0x09;
	data[0x09] = data[0x05];
	data[0x0A] = 0xFF;
	data[0x0B] = 0x00;
	data[0x0C] = 0xFF;

	this->SendEx (data);
}

void SYTEC200D::CallYMData ()
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x0A;
	data[0x02] = 0x0A;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x58;
	data[0x07] = 0x01;
	data[0x08] = 0x02;
	data[0x09] = data[0x05];
	data[0x0A] = 0x01;
	data[0x0B] = 0x00;
	data[0x0C] = 0x00;
	data[0x0D] = 0x00;

	this->SendEx (data);
}

void SYTEC200D::AnalysisFor2ndData(ST_BYTE * data)
{
	const ST_BYTE * dataptr = data + 7;
	ST_INT len = *dataptr;
	dataptr += 5;

	const DeviceDataArea * dataareas = 0;
	if (this->GetDevice() && this->GetDevice()->GetDeviceInfo()->DataAreasCount > 0)
		dataareas = this->GetDevice()->GetDeviceInfo()->DataAreas;

	ST_FLOAT value = 0;
	for (ST_INT index = 0; index < len; ++index, dataptr += sizeof(ST_INT16))
	{
		ST_INT16 transvalue = 0;
		memcpy(&transvalue, dataptr, sizeof (transvalue));
		value = transvalue;

		for (ST_INT it = 0; dataareas && it < dataareas->itemCount; ++it)
		{
			if (dataareas->items && dataareas->items[it].id == index + 10000
			 && dataareas->items[it].coeficient != 0)
				value *= dataareas->items[it].coeficient;
		}

		this->UpdateValue (index + 10000, value);
	}
}

void SYTEC200D::AnalysisForYXData  (ST_BYTE * data)
{
	const ST_BYTE * dataptr = data + 11;
	ST_INT len = 8 * 16;
	for (ST_INT index = 0; index < len; ++index)
	{
		if ((index % 8) == 0)
			++dataptr;
		ST_BYTE value = (bool)(*dataptr & (0x01 << (index % 8)));
		this->UpdateValue (index, value);
	}
}

// 未测试
void SYTEC200D::AnalysisForYMData  (ST_BYTE * data)
{
	const ST_BYTE * dataptr = data + 12;
	ST_INT len = 4;

	const DeviceDataArea * dataareas = 0;
	if (this->GetDevice() && this->GetDevice()->GetDeviceInfo()->DataAreasCount > 2)
		dataareas = this->GetDevice()->GetDeviceInfo()->DataAreas + 2;

	for (ST_INT index = 0; index < len; ++index, dataptr += sizeof(YMType))
	{
		YMType ymvar;
		memcpy (&ymvar, dataptr, sizeof(YMType));

		double value = ymvar.mv;
		for (ST_INT it = 0; dataareas && it < dataareas->itemCount; ++it)
		{
			if (dataareas->items && dataareas->items[it].id == index + 20000
			 && dataareas->items[it].coeficient != 0)
				value *= dataareas->items[it].coeficient;
		}

		this->UpdateValue (index + 20000, value);
	}
}

void SYTEC200D::AnalysisForSOE     (ST_BYTE * data)
{
	ST_BYTE num   = data[11] - 1;
	ST_BYTE statu = data[12];

	this->UpdateValue(num, statu);

	if (num == 96) m_totalcallcount = 0;
	if (num > 111) statu = 1;

	struct tm t_tm;
	DateTime::localtime (time(0), t_tm);

	ProtocolTask task;
	Strcpy(task.taskCmd,"SOE");
	task.isTransfer     = true;
	task.transChannelId = -1;
	task.channelId      = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
	task.deviceId       = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.taskCmdCode    = 0;
	task.taskParamLen   = 14;
	task.taskAddr       = num;
	task.taskValue      = statu;
	task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.ignoreBack     = 1;
	task.taskTime       = 1000;
	task.taskParam[0]   = (t_tm.tm_year + 1900) % 256;
	task.taskParam[1]   = (t_tm.tm_year + 1900) / 256;
	task.taskParam[2]   =  t_tm.tm_mon  + 1;
	task.taskParam[3]   =  t_tm.tm_mday;
	task.taskParam[4]   =  data[16];
	task.taskParam[5]   =  data[15];
	task.taskParam[6]   = (data[13] + data[14] * 256) / 1000;
	task.taskParam[7]   =((data[13] + data[14] * 256) % 1000) % 256;
	task.taskParam[8]   =((data[13] + data[14] * 256) % 1000) / 256;
	task.taskParam[9]   = statu;
	task.taskParam[10]  = num % 256;
	task.taskParam[11]  = num / 256;
	task.taskParam[12]  = task.taskAddr1 % 256;
	task.taskParam[13]  = task.taskAddr1 / 256;
	Transfer(&task);
}

void SYTEC200D::SetTime ()
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x0F;
	data[0x02] = 0x0F;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x06;
	data[0x07] = 0x81;
	data[0x08] = 0x08;
	data[0x09] = data[0x05];
	data[0x0A] = 0xFF;
	data[0x0B] = 0x00;

	time_t t_now = time(0);
	if (t_now < 0) return;
	struct tm t_tm;
	DateTime::localtime (t_now, t_tm);

	data[0x0C] =(t_tm.tm_sec * 1000) % 256;
	data[0x0D] =(t_tm.tm_sec * 1000) / 256;
	data[0x0E] = t_tm.tm_min;
	data[0x0F] = t_tm.tm_hour;
	data[0x10] = t_tm.tm_mday;
    data[0x11] = t_tm.tm_mon + 1;
    data[0x12] = t_tm.tm_year- 100;

    this->SendEx (data);
}

void SYTEC200D::YKSelect (const ProtocolTask& task)
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x0A;
	data[0x02] = 0x0A;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x40;
	data[0x07] = 0x01;
	data[0x08] = 0x12;
	data[0x09] = data[0x05];
	data[0x0A] = data[0x0B] = 0xFF;
	data[0x0C] = (task.taskValue ? 0x82 : 0x81);
	data[0x0D] = 0xEE;

	this->SendEx (data);
}

void SYTEC200D::YKExecut (const ProtocolTask& task)
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x0A;
	data[0x02] = 0x0A;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x40;
	data[0x07] = 0x01;
	data[0x08] = 0x12;
	data[0x09] = data[0x05];
	data[0x0A] = data[0x0B] = 0xFF;
	data[0x0C] = (task.taskValue ? 0x02 : 0x01);
	data[0x0D] = 0xEE;

	this->SendEx (data);
}

void SYTEC200D::YKCancel (const ProtocolTask& task)
{
	ST_BYTE data[64] = {0};
	data[0x00] = 0x68;
	data[0x01] = 0x0A;
	data[0x02] = 0x0A;
	data[0x03] = 0x68;
	data[0x04] = GetCtrlCode (0x03, true);
	data[0x05] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[0x06] = 0x40;
	data[0x07] = 0x01;
	data[0x08] = 0x12;
	data[0x09] = data[0x05];
	data[0x0A] = data[0x0B] = 0xFF;
	data[0x0C] = (task.taskValue ? 0x42 : 0x41);
	data[0x0D] = 0xEE;

	this->SendEx (data);
}
