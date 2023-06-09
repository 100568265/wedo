
#include "yd800.h"

#include "Device.h"
#include "Channel.h"
#include "EngineBase.h"

#include "datetime.h"
#include "syslogger.h"

#pragma pack(push,1)	// 紧凑对齐

struct CtrlCode {
	CtrlCode() : fc(0), fd(0), fa(0), prm(0), dir(0) {}

    inline operator unsigned char () { return *((unsigned char*)this); }

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

_PROTOCOL_INTERFACE_IMPLEMENT_(YD800)

YD800::YD800():
_has1stData(false),
_sendstate (0),
_old_state (0)
{}

YD800::~YD800()
{}

void YD800::Init()
{
	_has1stData	   = false;

	_sendstate     = 1;
	FCB            = 0;
	oldcurtime     = clock();
	_askserachtime  = clock();
	CLtime         = _askserachtime;
	BreakCallState = 0;
	sendflag       = 0;
}

void YD800::Uninit()
{}

void YD800::OnRead(ST_BYTE* pbuf, ST_INT& readed)
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

ST_BOOLEAN YD800::IsSupportEngine(ST_INT engineType)
{
    return engineType == EngineBase::ENGINE_POLLING;
}

ST_BOOLEAN	YD800::OnSend()
{
    Thread::SLEEP(50);

	if (_has1stData) {
		_has1stData = false;
		CtrlCode ctrl;
		ctrl.fc  = 0x0A; ctrl.fd  = 0x01;
		ctrl.fa  = FCB ; ctrl.prm = 0x01;
		SendShortFrame (ctrl);
		return true;
	}

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

	switch (_sendstate) {
		case 0x00: break;

		case 0x01: {	// 复位
			CtrlCode ctrl;
			ctrl.prm = 0x01;
			SendShortFrame (ctrl);
			StateTransition(0x08);
		} return true;
		// case 0x02: {	// 请求状态
		// 	CtrlCode ctrl;
		// 	ctrl.fc  = 0x09;
		// 	ctrl.fa  = FCB ; ctrl.prm = 0x01;
		// 	SendShortFrame (ctrl);
		//	StateTransition(0x04);
		// 	BreakCallState = 1;
		// } return true;
		case 0x03: {
			AskASDU0x15 (0x16, 0x04, 0x01);
			StateTransition(0x00);
		} return true;
		case 0x04: {	// 总查询
			SendAllSearch();
			StateTransition(0x06);
		} return true;
		case 0x06: {	// 召唤电度
			AskASDU0x15 (0x08, 0x06, 0x01);
			StateTransition(0x03);
		} return true;
		case 0x08: {	// 读事件信息组
			AskASDU0x15 (0x00, 0x08, 0x0A);
			StateTransition(0x09);
		} return true;
		case 0x09: {	// 读告警信息组
			AskASDU0x15 (0x01, 0x09, 0x0A);
			StateTransition(0x11);
		} return true;
		case 0x11: {	// 读录波状态量
			AskASDU0x15 (0x02, 0x11, 0x0A);
			StateTransition(0x10);
		} return true;
		case 0x10: {	// 读录波通道描述表
			AskASDU0x15 (0x03, 0x10, 0x0A);
			StateTransition(0x20);
		} return true;
		case 0x20: {	// 读录波通道单位表
			AskASDU0x15 (0x04, 0x10, 0x09);
			StateTransition(0x30);
		} return true;
		case 0x30: {
			AskASDU0x15 (0x05, 0x10, 0x06);
			StateTransition(0x04);
		} return true;
	}

	struct tm now_tm;
	DateTime::localtime (time(0), now_tm);
	newday = now_tm.tm_mday;
	if (newday != oldday)  //发对时
	{
		oldday = newday;
		if (now_tm.tm_sec == 0x00)
		{
			SendASDU6();
			Thread::SLEEP (1000);
			return false;
		}

	}

	clock_t  newclock_ = clock();

	if ((abs(newclock_ - _askserachtime) > 1800 * CLOCKS_PER_SEC)) //大于1800秒就是30分钟
	{
		_askserachtime = newclock_;
		SendAllSearch();
		StateTransition(0x06);
		return true;
	}

	// if (BreakCallState)
	// {
	// 	SendAllSearch();
	// 	BreakCallState = 0;
	// 	return true;
	// }

	// if (abs(newclock_ - oldcurtime) > 50 * CLOCKS_PER_SEC) //大于3m
	// {
	// 	CtrlCode ctrl;
	// 	ctrl.prm = 0x01;
	// 	SendShortFrame (ctrl);
	// 	_sendstate = 0x01;
	// 	BreakCallState = 1;
	// 	return true;
	// }
//		if (abs(newclock_ - CLtime) > 7 * CLOCKS_PER_SEC)//发送测量数据
//		{
//			CLtime = newclock_;
//			SendASDU21();
//			return true;
//		}

	CtrlCode ctrl;
	ctrl.fc  = 0x0B; ctrl.fd  = 0x01;
	ctrl.fa  = FCB ; ctrl.prm = 0x01;
	SendShortFrame (ctrl);
	return true;
}

ST_BOOLEAN	YD800::OnProcess(ST_BYTE * pbuf, int len)
{
	oldcurtime = clock();

	FCB = !FCB;

	if (pbuf[0] == 0x10) //固定帧
	{
		if (pbuf[1] & 0x20) {
			this->Continue();
			return _has1stData = true;  //ACD为1代表要召唤一级数据
		}

		// if (BreakCallState)
		// {
		// 	BreakCallState = 0;
		// 	_sendstate = 4;
		// 	return true;
		// }
		ST_BYTE Recode = pbuf[1] & 0x0F;
        switch (Recode) {
            case 0x00: {    // 确认
            } break;
            case 0x01: {    // 链路忙
            } break;
            case 0x09: {    // 从站没有所召唤的数据
            	if (_sendstate == 0x04)
					StateTransition(0x01);
            } break;
            case 0x0B: {    // 从站以链路状态响应主站请求
            } break;
			case 0x0E:
			case 0x0F: {
				StateTransition(0x01);
			} break;
        }

//		_sendstate = 0x00;
	}

	if (pbuf[0] == 0x68) //可变帧
	{
		switch (pbuf[6]) { // type
			case 0x05: AnalysisASDU0x05 (pbuf); break;
            case 0x09: AnalysisASDU0x09 (pbuf); break;
			case 0x0A: AnalysisASDU0x0A (pbuf); break;
			// case 0x0A: AnalysisASDU15 (pbuf); break;
            case 0x2A: AnalysisASDU0x2A (pbuf); break;

            // case 0x32://遥测
            //     EXpainYc(pbuf);
            //     break;
            // case 0x2c://全遥信
            //     EXpainYx(pbuf);
            //     break;
            // case 0x28://变位遥信
            //     EXpainBwYx(pbuf);
            //     break;
            // case 0x29://SOE
            //     EXpainSOE(pbuf);
            //     break;
            // case 0x01:
            //     ASDU1(pbuf);//告警
            //     break;
            // case 0x02:
            //     ASDU2(pbuf);//保护动作信息
            //     break;
		}

		if (pbuf[4] & 0x20) {
		    this->Continue();
			_has1stData = true;  //ACD为1代表要召唤一级数据
		}
//		_sendstate = 0x00;
	}
	return true;
}

void YD800::SendShortFrame (ST_BYTE code) //发送固定帧
{
	ST_BYTE sendbuf[8] = {0};
	sendbuf[0] = 0x10;
	sendbuf[1] = code;
	sendbuf[2] = this->GetDevice()->GetDeviceInfo()->Address;

	sendbuf[3] = get_check_sum(sendbuf);
	sendbuf[4] = 0x16;
	this->Send(sendbuf, 5);
}

void YD800::AnalysisASDU0x05 (ST_BYTE * data)
{
	this->ShowMessage ("Recv ASDU05 frame.");
}

void YD800::AnalysisASDU0x09 (ST_BYTE * data)
{
	int count = data[7] &  0x7F;
	ST_UINT16 * dptr = (ST_UINT16*)(data + 12);
	ST_UINT16 tmpvar = 0;
	for (int i = 0; i < count; ++i, ++dptr)
	{
		memcpy (&tmpvar, dptr, sizeof(tmpvar));
		this->UpdateValue (10000 + i, (float)tmpvar);
	}
}

/*
const char * trans_str = "0123456789ABCDEF";
std::string gethexbytes(ST_BYTE * data, int len)
{
    std::string str;
    for (int i = 0; i < len; ++i)
    {
        str += trans_str[data[i] / 16];
        str += trans_str[data[i] % 16];
        str += ' ';
    }
    return str;
}
*/

void YD800::AnalysisASDU0x0A (ST_BYTE * data)
{
//    bool is_err = false;

	int count = data[13] & 0x7F;

	ST_BYTE * dptr = data + 14;
	for (int i = 0; i < count; ++i, dptr += (dptr[4] + 6))
	{
		switch (*dptr)
		{
			case 0x01: {
				ST_UINT16 value = dptr[6] + dptr[7] * 256;
				this->UpdateValue (dptr[1] + 9999, (float)value);
			} break;
			case 0x03: {
				if (m_bTask) {
					if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
					{
						m_curTask.taskResult.resultCode = 0;
						m_curTask.isTransfer = 1;
						Transfer(&m_curTask);
						memset(&m_curTask, 0, sizeof(m_curTask));
					}
				}
			} break;
			case 0x04: {	// 定值实际值
				float value = 0;
				if (dptr[3] == 0x0A)
				{
					value = dptr[6];
				}
				if (dptr[3] == 0x07)
				{
					memcpy(&value, dptr + 6, sizeof (value));
				}
				this->UpdateValue(dptr[1] + 14999, value);
			} break;
			case 0x08: {	// 动作事件
			} break;
			case 0x09: {	// 告警事件
			} break;
			case 0x18: {	// SOE事件
			    bool    value = (dptr[6] == 2 ? true: false);
			    ST_INT32 addr =  dptr[1] - 1;
				TransferEx (value, addr, dptr[10], dptr[9], dptr[7] + dptr[8] * 256);
                this->UpdateValue(addr, value);
			} break;
			case 0x06: {
				ST_UINT32 value = 0;
				memcpy (&value, dptr + 6, sizeof(value));
				this->UpdateValue (dptr[1] + 19999, value);
			} break;
			case 0x11: {
			} break;
			case 0x10: {
			} break;

			default: {
//			    is_err = true;
				this->ShowMessage ("Unknown message group.");
			} break;
		}
	}

	if (!count && m_bTask) {
		if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
			m_curTask.isTransfer = 1;
			Transfer(&m_curTask);
			memset(&m_curTask, 0, sizeof(m_curTask));
		}
	}
//	if (is_err)
//        SysLogger::GetInstance()->LogDebug("%s", gethexbytes(data, data[1] + 6).c_str());

}

// void YD800::AnalysisASDU15 (ST_BYTE * data)
// {
// 	this->ShowMessage ("Recv ASDU15 frame.");
// }

void YD800::AnalysisASDU0x2A (ST_BYTE * data)
{
	int count = data[7] & 0x7F;
	ST_BYTE * dptr = data + 12;
	for (int i = 0; i < count; ++i, ++dptr)
	{
		this->UpdateValue (i, (*dptr == 2 ? true: false));
	}
}
/*
void YD800::EXpainYc(ST_BYTE * Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[7]&0x7f;
	float fvalue=0;
	for (int i=0;i<yccount;i++)
	{
		funit=(Rebuf[12]&0x80)>>7;
		fvalue=((Rebuf[12+2*i]*256+Rebuf[13]+2*i)>>3)&0x1fff;
		if (funit)
		{
			fvalue=-1*fvalue;
		}
		this->UpdateValue(10000+i,fvalue);
	}
}
void    YD800::EXpainYx(ST_BYTE* Rebuf)
{
	ST_BYTE yxcount,funit;
	ST_BYTE bvalue;
	yxcount=Rebuf[7]&0x7f;
	for (int k=0;k<yxcount;k++)
	{
		for (int i=0;i<8;i++)
		{
			bvalue=(Rebuf[12+k]>>i)&0x01;
			this->UpdateValue(i+8*k,bvalue);
		}
	}
}
void    YD800::EXpainBwYx(ST_BYTE* Rebuf)
{

}
void    YD800::EXpainSOE(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	inf=Rebuf[11]-110;
	ST_BYTE bvalue;
	bvalue=Rebuf[12];
	int nhour,nminte,nmillsecond;
	nhour=Rebuf[16];
	nminte=Rebuf[15];
	nmillsecond=Rebuf[13]+Rebuf[14]*256;

	TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
}
*/
void     YD800::SendAllSearch()
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

    ST_BYTE sendbuf[20] = {0};

    sendbuf[ 0] = 0x68;
	sendbuf[ 1] = 0x09;
	sendbuf[ 2] = 0x09;
	sendbuf[ 3] = 0x68;
	sendbuf[ 4] = ctrl;
	sendbuf[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[ 6] = 0x07; //adsu
	sendbuf[ 7] = 0x81; //vsq
	sendbuf[ 8] = 0x09;
	sendbuf[ 9] = sendbuf[5];
	sendbuf[10] = 0xff; //fun
	sendbuf[11] = 0x00;  //inf
	sendbuf[12] = 0x06;  //序号
	sendbuf[13] = get_check_sum(sendbuf);
	sendbuf[14] = 0x16;

	this->Send(sendbuf, 15);
}

void	YD800::SendASDU21()
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

	ST_BYTE sendbuf[20] = {0};
	sendbuf[ 0] = 0x68;
	sendbuf[ 1] = 0x0d;
	sendbuf[ 2] = 0x0d;
	sendbuf[ 3] = 0x68;
	sendbuf[ 4] = ctrl;
	sendbuf[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[ 6] = 0x15; //adsu
	sendbuf[ 7] = 0x81; //vsq
	sendbuf[ 8] = 0x2a;
	sendbuf[ 9] = sendbuf[5];
	sendbuf[10] = 0xfe; //fun
	sendbuf[11] = 0xf1;  //inf //读遥测
	sendbuf[12] = 0x00;  //Rii
	sendbuf[13] = 0x01;  //NOG
	if (this->GetDevice()->GetDeviceInfo()->DataAreas && this->GetDevice()->GetDeviceInfo()->DataAreasCount > 2)
		sendbuf[14]=this->GetDevice()->GetDeviceInfo()->DataAreas[2].readCode;  //GIN 09 为保护数据， 0d为测量数据
	sendbuf[15] = 0x00;  //GIN
	sendbuf[16] = 0x01;  //KOD
	sendbuf[13] = get_check_sum(sendbuf);
	sendbuf[18] = 0x16;
	 this->Send(sendbuf, 19);
}

// 68 0D 0D 68 73 01 15 81 2A 01 FE F1 XX 01 XX XX XX XX 16
void YD800::AskASDU0x15 (ST_BYTE rii, ST_UINT16 gin, ST_BYTE kod)
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

	ST_BYTE sendbuf[20] = {0};
	sendbuf[ 0] = 0x68;
	sendbuf[ 1] = 0x0D;
	sendbuf[ 2] = 0x0D;
	sendbuf[ 3] = 0x68;
	sendbuf[ 4] = ctrl;
	sendbuf[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[ 6] = 0x15;			//adsu type
	sendbuf[ 7] = 0x81;			//vsq
	sendbuf[ 8] = 0x2A;
	sendbuf[ 9] = sendbuf[5];
	sendbuf[10] = 0xFE;			//FUN
	sendbuf[11] = 0xF1;			//INF
	sendbuf[12] = rii;			//RII
	sendbuf[13] = 0x01;			//NOG
    sendbuf[14] = gin % 256;	//GIN
	sendbuf[15] = gin / 256;
	sendbuf[16] = kod;			//KOD

	sendbuf[17] = get_check_sum(sendbuf);
	sendbuf[18] = 0x16;
	 this->Send(sendbuf, 19);
}
/*
void    YD800::ASDU2a(ST_BYTE* Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[7]&0x7f;
	ST_BYTE fvalue=0;

	for (int i=0;i<yccount;i++)
	{
		fvalue=Rebuf[12+i];
		if (Rebuf[11]==0x20) //压板
		{
			this->UpdateValue(i,(ST_BYTE)fvalue);
		}
		else if (Rebuf[11]==0x40)//开入遥信
		{
			this->UpdateValue(i+10,(ST_BYTE)fvalue);
		}
	}

}


void	YD800::ASDU10(ST_BYTE* Rebuf)
{
	ST_BYTE yccount,funit;
	yccount=Rebuf[13]&0x7f;
	float fvalue=0;
	for (int i=0;i<yccount;i++)
	{
		if (Rebuf[14+i*10]==0x09) //保护测量
		{
			char bvalue[4];
			bvalue[0]=Rebuf[i*10+20];
			bvalue[1]=Rebuf[i*10+1+20];
			bvalue[2]=Rebuf[i*10+2+20];
			bvalue[3]=Rebuf[i*10+3+20];
			fvalue =*(float*)(bvalue);
			this->UpdateValue(10000+i,fvalue);
		}
		else if (Rebuf[14+i*10]==0x0d) //测量数据
		{
			char bvalue[4];
			bvalue[0]=Rebuf[i*10+20];
			bvalue[1]=Rebuf[i*10+1+20];
			bvalue[2]=Rebuf[i*10+2+20];
			bvalue[3]=Rebuf[i*10+3+20];
			fvalue =*(float*)(bvalue);
			this->UpdateValue(10000+i,fvalue);
		}

	}
}


void	YD800::ASDU1(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	if (Rebuf[11]>=176 &&Rebuf[11]<=239 ) //SOE
	{
		inf=abs(Rebuf[11]-176);
		ST_BYTE bvalue;
		bvalue=Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;

		TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
	}
	else if (Rebuf[11]>=32 &&Rebuf[11]<=41 ) //压板
	{
		this->UpdateValue(Rebuf[11]-32,Rebuf[12]);
		inf=abs(Rebuf[11]-32);
		ST_BYTE bvalue;
		bvalue=Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;

		TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
	}
	else if (Rebuf[11]>=64 &&Rebuf[11]<=92 ) //开入
	{
		inf=abs(Rebuf[11]-64+10);
		ST_BYTE bvalue;
		bvalue=Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16]&0x7f;
		nminte=Rebuf[15]&0x1f;
		nmillsecond=Rebuf[13]+Rebuf[14]*256;

		this->UpdateValue(Rebuf[11]-64+10,Rebuf[12]);
		TransferEx (bvalue, inf, nhour, nminte, nmillsecond);
	}
}

void	YD800::ASDU2(ST_BYTE* Rebuf)
{
	ST_BYTE fun,inf;
	fun=Rebuf[10];
	if (Rebuf[11]>=203 &&Rebuf[11]<=239 ) //SOE
	{
		inf=abs(Rebuf[11]-176);
		ST_BYTE bvalue;
		bvalue=Rebuf[12];
		int nhour,nminte,nmillsecond;
		nhour=Rebuf[16+4]&0x7f;
		nminte=Rebuf[15+4]&0x1f;
		nmillsecond=Rebuf[13+4]+Rebuf[14+4]*256;

		TransferEx (bvalue, inf + 5000, nhour, nminte, nmillsecond);
	}
}
*/
void YD800::SendASDU6()
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

	ST_BYTE sendbuf[21] = {0};
	sendbuf[ 0] = 0x68;
	sendbuf[ 1] = 0x0F;
	sendbuf[ 2] = 0x0F;
	sendbuf[ 3] = 0x68;
	sendbuf[ 4] = ctrl;
	sendbuf[ 5] = 0xFF;//this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[ 6] = 0x06; //adsu
	sendbuf[ 7] = 0x81; //vsq
	sendbuf[ 8] = 0x08;
	sendbuf[ 9] = sendbuf[5];
	sendbuf[10] = 0xFF; //fun
	sendbuf[11] = 0x00;  //inf
	struct tm now_tm;
	DateTime::localtime (time(0), now_tm);
	sendbuf[12] =(now_tm.tm_sec * 1000) % 256; //mS
	sendbuf[13] =(now_tm.tm_sec * 1000) / 256; //ms
	sendbuf[14] = now_tm.tm_min;  //min
	sendbuf[15] = now_tm.tm_hour; //hour
	sendbuf[16] = now_tm.tm_mday; //day
	sendbuf[17] = now_tm.tm_mon + 1;  //mon
	sendbuf[18] = now_tm.tm_year - 100;  //year

	sendbuf[19] = get_check_sum(sendbuf);
	sendbuf[20] = 0x16;
	this->Send(sendbuf, 21);
}

void    YD800::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
{
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
	task.taskAddr       = addr;
	task.taskValue      = statu;
	task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.ignoreBack     = 1;
	task.taskTime       = 1000;
	task.taskParam[0]   = (t_tm.tm_year + 1900) % 256;
	task.taskParam[1]   = (t_tm.tm_year + 1900) / 256;
	task.taskParam[2]   =  t_tm.tm_mon  + 1;
	task.taskParam[3]   =  t_tm.tm_mday;
	task.taskParam[4]   =  hour;
	task.taskParam[5]   =  min;
	task.taskParam[6]   =  msec / 1000;
	task.taskParam[7]   = (msec % 1000) % 256;
	task.taskParam[8]   = (msec % 1000) / 256;
	task.taskParam[9]   = statu;
	task.taskParam[10]  = addr % 256;
	task.taskParam[11]  = addr / 256;
	task.taskParam[12]  = task.taskAddr1 % 256;
	task.taskParam[13]  = task.taskAddr1 / 256;
	Transfer(&task);
}

void YD800::YKSelect(ProtocolTask & task)
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

	ST_BYTE data[64] = {0};
	data[ 0] = 0x68;
	data[ 1] = 0x11;
	data[ 2] = 0x11;
	data[ 3] = 0x68;
	data[ 4] = ctrl;
	data[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	data[ 6] = 0x0A;
	data[ 7] = 0x81;
	data[ 8] = 0x28;
	data[ 9] = data[5];
	data[10] = 0xFE;
	data[11] = 0xF9;
	data[12] = 0x03;
	data[13] = 0x01;
	data[14] = 0x03;
	data[15] = 0x01;
	data[16] = 0x01;
	data[17] = 0x03;
	data[18] = 0x01;
	data[19] = 0x01;
	data[20] = (task.taskValue == 2 ? 0x02: 0x01);
	data[21] = get_check_sum(data);
	data[22] = 0x16;

	this->Send(data, 23);
}

void YD800::YKExecut(ProtocolTask & task)
{
    CtrlCode ctrl;
    ctrl.fc  = 0x03; ctrl.fd  = 0x01;
    ctrl.fa  = FCB ; ctrl.prm = 0x01;

	ST_BYTE data[64] = {0};
	data[ 0] = 0x68;
	data[ 1] = 0x11;
	data[ 2] = 0x11;
	data[ 3] = 0x68;
	data[ 4] = ctrl;
	data[ 5] = this->GetDevice()->GetDeviceInfo()->Address;
	data[ 6] = 0x0A;
	data[ 7] = 0x81;
	data[ 8] = 0x28;
	data[ 9] = data[5];
	data[10] = 0xFE;
	data[11] = 0xFA;
	data[12] = 0x04;
	data[13] = 0x01;
	data[14] = 0x03;
	data[15] = 0x01;
	data[16] = 0x01;
	data[17] = 0x03;
	data[18] = 0x01;
	data[19] = 0x01;
	data[20] = (task.taskValue == 2 ? 0x02: 0x01);
	data[21] = get_check_sum(data);
	data[22] = 0x16;

	this->Send(data, 23);
}

void YD800::YKCancel(ProtocolTask & task)
{

}
