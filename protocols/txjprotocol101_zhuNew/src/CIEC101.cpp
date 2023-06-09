#include "CIEC101.h"
#include "sysmutex.h"
#include "Channel.h"
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
volatile static ST_BOOLEAN debugFlag = false;
volatile static ST_BOOLEAN fcbBit = true;
volatile static ST_BOOLEAN fcvBit = false;
volatile static ST_BOOLEAN prmBit = false;


uint16_t GetCheckSum (const uint8_t *pdata)
{
    uint16_t sum =0;
    uint16_t len =0;
    if(*pdata ==0x68){
        len = pdata[1];
        pdata += 4;
    }
    if(*pdata ==0x10){
        len =2;
        pdata +=1;
    }
    for(uint16_t i=0;i<len;i++){
        sum += pdata[i];
    }
    return sum;
}


CIEC101* CreateInstace()
{
	return new CIEC101 ();
}

CIEC101::CIEC101()
{
    //ctor
}

CIEC101::~CIEC101()
{
    //dtor
}

void CIEC101::Init ()
{
    m_curreadIndex = 0;
    Newcurtime=clock();
	oldcurtime=clock();
	sendserchtime=clock();
	CLtime=sendserchtime;
	m_b0701 = 0;
	// this->GetDevice()->GetDeviceInfo()->AddressEx;
}

void CIEC101::Uninit()
{
}

volatile static ST_BOOLEAN is1stData = false;
volatile static CIEC101::StepTypre step = CIEC101::Undefined;
inline static ST_BYTE GetFCB ()
{
	return (fcbBit ? 0x70 : 0x50);
}


ST_BOOLEAN CIEC101::IsSupportEngine (ST_INT engineType)
{
	return 1 == engineType; // EngineType == Fulling;
}

void	CIEC101::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
    if(!this->GetCurPort())
    {
        return;
    }
    if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
	{
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
    ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 2000 ? interval : 2000);
	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
    if(len < 5) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	ST_BYTE frametype = 0;
	for(; star < len; ++star) {
		if((pbuf[star] ==0x10) || (pbuf[star] ==0x68))
		{
            frametype = pbuf[star];
				break;
		}
	}
    if(star > 0) {
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
    if(star == len) {
		//全是乱码
		ShowMessage ("Garbled code, clear buffer.");
		this->GetCurPort()->Clear();
		return;
	}
	if(frametype == 0x10)
	{
	    len = this->GetCurPort()->PickBytes(pbuf, 5, 2000);
	    if(len < 5)
	    {
            ShowMessage("frametupe=0x10,A data length is not enough , continue to receive.");
            return;
        }
        if(GetCheckSum(pbuf) != pbuf[3]){
            this->GetCurPort()->Clear();
            ShowMessage ("Check sum error.");
            return;
        }
        if(pbuf[0] ==0x10 &&(pbuf[4]==0x16)){
            readed = this->GetCurPort()->ReadBytes(pbuf,5);
        }
        else{
            this->GetCurPort()->Clear();
            ShowMessage ("Garbled short frame of discarding garbage..");
        }
    }
    if(frametype ==0x68)
    {
        len = this->GetCurPort()->PickBytes(pbuf, 10, 2000);
        if(len<5){
            ShowMessage ("frametupe=0x68,A data length is not enough, continue to receive.");
            return;
        }
        if(pbuf[0]==0x68 &&pbuf[3] ==0x68 &&pbuf[1]==pbuf[2]){
            ST_INT datalen = pbuf[1] + 6;
            len = this->GetCurPort()->PickBytes(pbuf,datalen,2000);

            if(len>=datalen){
                readed = this->GetCurPort()->ReadBytes(pbuf,datalen);

                if(GetCheckSum(pbuf)!=pbuf[readed-2]){
                    readed =0;
                    ShowMessage ("Check sum error.");
                    return;
                }
                if(pbuf[readed-1]!=0x16){
                    readed = 0;
                    this->GetCurPort()->Clear();
                    ShowMessage ("Variable length frame footer error, the system will discard the entire frame.");
                }
            }
            else{
                this->GetCurPort()->Clear();
                ShowMessage ("Variable length data frame length is not enough, continue to receive.");

            }
        }
        else{
            this->GetCurPort()->Clear();
            ShowMessage ("Garbled variable length frame, the frame is discarded.");
        }
    }

}
ST_BOOLEAN	CIEC101::OnSend()
{

        if(this->HasTask()&&this->GetTask(&m_curTask))
        {
            if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
            {
                switch(m_curTask.taskCmdCode){
                    case 0:sendqueue.push(REMOTE_CTRL_SELECT);
                            break;
                    case 1:sendqueue.push(REMOTE_CTRL_EXEC);
                            break;
                    case 2:sendqueue.push(REMOTE_CTRL_CANCEL);
                            break;
                    default:break;
                }
            }
        }
        if(!m_isCommStart)
        {
            m_isCommStart = true;
            sendqueue.push(RE_ASK_LINK_STATE);
            return true;
        }

        Newcurtime=clock();
		if ((abs(Newcurtime-sendserchtime) > 1200 * CLOCKS_PER_SEC)) //大于1200秒就是20分钟
		{
			sendserchtime=Newcurtime;
			sendqueue.push(PROOFING_TIME);
		}
		if (abs(Newcurtime-oldcurtime)>600 * CLOCKS_PER_SEC) //大于10min
		{
            oldcurtime = Newcurtime;
            sendqueue.push(TOTAL_CALL);
		}
		if (abs(Newcurtime-CLtime)>1800 * CLOCKS_PER_SEC)//发送测量数据
		{
            CLtime = Newcurtime;
            sendqueue.push(TOTAL_CALL_PH);
		}

		if(sendqueue.empty())
		{
            return true;
        }

        StepTypre index = sendqueue.front();
        sendqueue.pop();

        fcvBit = false;
        switch (index){
            case RE_ASK_LINK_STATE:{
                 step = RE_ASK_LINK_STATE;
            }
            case ASK_LINK_STATE:{
                 Thread::SLEEP(1000);
                MainAskLinkStatu();
            }break;

            case RE_RST_LINK:
                step = RE_RST_LINK;

            case RST_LINK:{
                MainResetLink();
            }break;
            case RE_TOTAL_CALL:
                 step = RE_TOTAL_CALL;
            case TOTAL_CALL:{
                fcbBit = true;
                Thread::SLEEP(50);
                MainAskAllCmd (3| GetFCB());
                }break;
            case TOTAL_CALL_PH:{
                fcvBit = true;
                CallPH(3 | GetFCB());
                }break;
            case ASK_1ST_DATA:{
                fcvBit = true;
                CallClass1(10 | GetFCB());
                }break;
            case ASK_2ND_DATA: {
                fcvBit = true;
                CallClass2(11 | GetFCB());
                } break;
            case REMOTE_CTRL_SELECT:{
                fcvBit = true;
                SendPreYK((3|GetFCB()),m_curTask.taskAddr + 0x12C, m_curTask.taskValue);
                }break;
            case REMOTE_CTRL_EXEC: {
                fcvBit = true;
                SendYK((3 | GetFCB()), m_curTask.taskAddr + 0x12C, m_curTask.taskValue);
                } break;
        	case REMOTE_CTRL_CANCEL: {
                fcvBit = true;
                SendEndYK((3 | GetFCB()), m_curTask.taskAddr + 0x12C, m_curTask.taskValue);
                } break;
            case PROOFING_TIME: {
                fcvBit = true;
                SendTime(3| GetFCB());
                } break;
            default: ShowMessage("Circuit be disrupted");
                break;
        }

        fcbBit = (fcvBit ? !fcbBit : fcbBit);

    return true;
}
ST_BOOLEAN	CIEC101::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    m_ivRead = clock();

	if (len == 1 && *pbuf == 0xE5) {
		switch (step) {
		case RE_RST_LINK: {
			sendqueue.push(RE_TOTAL_CALL);
			step = Undefined;
		} return true;
		default: {
			if (sendqueue.empty())
				sendqueue.push(ASK_2ND_DATA);
		} return true;
		}
	}
		unsigned char fc_it = 0;
	if (len == 0x05) fc_it = 1;
	else fc_it = 4;

	if (!(prmBit = ST_BOOLEAN(pbuf[fc_it] & 0x40))) // 获取启动报文位, 1:启动站，报文为发送或请求报文。
		is1stData = ST_BOOLEAN(pbuf[fc_it] & 0x20);  // 获取要求访问位
    switch (pbuf[fc_it] & 0x0F) { // 功能码
		// 肯定认可 或 复位链路
	case FC::CONFIRM: { // FC::RST_LINK
		if (prmBit) SendAck(0x80);
		else {
			switch (step) {
			case RE_RST_LINK: {
				sendqueue.push(RE_TOTAL_CALL);
				step = Undefined;
			} break;
			default:
				break;
			}
		}
	} break;
		// 链路忙
	case FC::LINK_BUSY:
		ShowMessage("Link is busy, Repeat link reset operation.");
		sendqueue.push(RST_LINK);
		break;
		// 发送数据
	case FC::SEND_DATA:
		SendAck(0x80);
		break;
		// 响应用户数据
	case FC::ANSWER_DATA: { //FC::VISIT_ASK
		if (prmBit) SendAck(0x80);
	} break;
		// 无此用户数据
	case FC::NO_ANSWER_DATA: {// FC::ASK_LINK_STATE
		if (prmBit) SubAskLinkStatu();
	} break;
		// 响应链路状态-链路完好
	case FC::LINK_PERFECT: {// or FC::ASK_TIER2_DATA
		if (prmBit) SendAck(0x80);
		else {
			switch (step) {
			case RE_ASK_LINK_STATE: {
				sendqueue.push(RE_RST_LINK);
				step = Undefined;
			} break;
			default:
				break;
			}
		}
	} break;
		// 链路未工作
	case FC::LINK_NO_WORK: {
		ShowMessage("Link is not working, Reset link.");
		sendqueue.push(RST_LINK);
	} break;
		// 链路还未准备好
	case FC::LINK_NO_FINISH: {
		ShowMessage("Link is not finish, Repeat link reset operation.");
		sendqueue.push(ASK_LINK_STATE);
	} break;
	default:
		sendqueue.push(RE_ASK_LINK_STATE);
		break;
	}
	if (is1stData) {
		is1stData = false;
		sendqueue.push(ASK_1ST_DATA);
	}
	else if (!prmBit) {
		if (sendqueue.empty())
			sendqueue.push(ASK_2ND_DATA);
	}
    	if (*pbuf == 0x68) {
		switch (pbuf[6]) {// 信息体对象类型
		case TYP::M_SP_NA_1: {//单点信息
			Explain_M_SP_NA_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_SP_TA_1: {//带时标的单点信息
			Explain_M_SP_TA_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_DP_NA_1: {
			Explain_M_DP_NA_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_DP_TA_1: {//带时标的双点信息
			Explain_M_DP_TA_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_SP_TB_1: {//带CP56Time2a时标的单点信息
			Explain_M_SP_TB_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_DP_TB_1: {//带CP56Time2a时标的双点信息
			Explain_M_DP_TB_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_ME_NA_1: {//测量值，规一化值
			Explain_M_ME_NA_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_ME_NB_1: {//测量值，标度化值
			Explain_M_ME_NB_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_ME_NC_1: {//测量值，短浮点数
			Explain_M_ME_NC_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_ME_ND_1: {//测量值，不带品质描述词的规一化值
			Explain_M_ME_ND_1(&pbuf[7], len - 8);
		} break;
		case TYP::M_IT_NA_1: {//电能脉冲量
			Explain_M_IT_NA_1(&pbuf[7], len - 8);
		} break;
		case TYP::C_SC_NA_1: {//单点遥控返回
			//if ((pbuf[8] & 0x3F) == COT::ACTCON) {
				this->m_curTask.taskResult.resultCode = 0;
				Transfer(&m_curTask);
                memset(&this->m_curTask,0,sizeof(m_curTask));
			//}
		} break;
		case TYP::C_DC_NA_1: { //双点遥控返回
			if ((pbuf[8] & 0x3F) == COT::ACTCON) {
				this->m_curTask.taskResult.resultCode = 0;
				Transfer(&m_curTask);
                memset(&this->m_curTask,0,sizeof(m_curTask));
			}
		} break;
		case TYP::C_IC_NA_1: { // 总召唤返回
			switch (pbuf[8] & 0x3F) {
			case COT::ACTCON: {
				if (!prmBit) sendqueue.push(ASK_1ST_DATA);
			} break;
			case COT::ACTTERM: {
				switch (step) {
				case RE_TOTAL_CALL: {
					sendqueue.push(TOTAL_CALL_PH);
					step = Undefined;
				}
				default:
					break;
				}
			} break;
			default:
				break;
			}
		} break;
		case TYP::C_CI_NA_1: {
			switch (pbuf[8] & 0x3F) {
			case COT::ACTCON: {
				if (!prmBit) sendqueue.push(ASK_2ND_DATA);
			} break;
			case COT::ACTTERM: {
			}
			default:
				break;
			}
		} break;
		default:
			break;
		}
	}

    return true;
}

void  CIEC101::MainAskLinkStatu()
{
        ST_BYTE sendbuf[256];
        sendbuf[0] = 0x10;
        sendbuf[1] = 0x49;
        sendbuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
        sendbuf[3] = GetCheckSum(sendbuf);
        sendbuf[4] = 0x16;
        this->Send(sendbuf,5);
}

void  CIEC101::MainResetLink()
{
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x10;
    sendbuf[1] = 0x40;
    sendbuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[3] = GetCheckSum(sendbuf);
    sendbuf[4] = 0x16;
    this->Send(sendbuf,5);
}
void  CIEC101::MainAskAllCmd(ST_BYTE fc)
{
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x68;
    sendbuf[1] = 0x09;
    sendbuf[2] = 0x09;
    sendbuf[3] = 0x68;
    sendbuf[4] = fc;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = 0x64;
    sendbuf[7] = 0x01;
    sendbuf[8] = 0x06;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10]= 0x00;
    sendbuf[11]= 0x00;
    sendbuf[12]= 0x14;
    sendbuf[13]= GetCheckSum(sendbuf);
    sendbuf[14]= 0x16;
    this->Send(sendbuf,15);
}
void  CIEC101::CallPH(ST_BYTE fc)//召唤电度量
{
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x68;
    sendbuf[1] = 0x09;
    sendbuf[2] = 0x09;
    sendbuf[3] = 0x68;
    sendbuf[4] = fc;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = 0x65;
    sendbuf[7] = 0x01;
    sendbuf[8] = 0x06;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10]= 0x00;
    sendbuf[11]= 0x00;
    sendbuf[12]= 0x05;
    sendbuf[13]= GetCheckSum(sendbuf);
    sendbuf[14]= 0x16;
    this->Send(sendbuf,15);
}


void  CIEC101::CallClass1(ST_BYTE byCID)
{
        ST_BYTE sendbuf[256];
        sendbuf[0] = 0x10;
        sendbuf[1] = byCID;
        sendbuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
        sendbuf[3] = GetCheckSum(sendbuf);
        sendbuf[4] = 0x16;
        this->Send(sendbuf,5);
}

void  CIEC101::CallClass2(ST_BYTE byCID)
{
        ST_BYTE sendbuf[256];
        sendbuf[0] = 0x10;
        sendbuf[1] = byCID;
        sendbuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
        sendbuf[3] = GetCheckSum(sendbuf);
        sendbuf[4] = 0x16;
        this->Send(sendbuf,5);
}

void  CIEC101::SendYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn)
{
    ST_BYTE sendbuf[256];
	if (fc==0x2d) //单点
	{
		sendbuf[12] = bYkOn?0x01:0x00;
	}
	else if (fc==0x2e)
	{
		sendbuf[12] = bYkOn?0x02:0x01;
	}

	sendbuf[0] = 0x68;
	sendbuf[1] = 0x09;
	sendbuf[2] = 0x09;
	sendbuf[3] = 0x68;
	sendbuf[4] = fc;
	sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6] = 0x2d;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x06;
	sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10] = wAddr;
	sendbuf[11] = wAddr>>8;
	sendbuf[12] = bYkOn?0x01:0x00;
	sendbuf[13] = GetCheckSum(sendbuf);
	sendbuf[14] = 0x16;
	this->Send(sendbuf,15);
	ShowMessage("Remote control execute");
}
void  CIEC101::SendEndYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn)
{
    ST_BYTE sendbuf[256];
	if (fc==0x2d) //单点
	{
		sendbuf[12] = bYkOn?0x81:0x80;
	}
	else if (fc==0x2e)
	{
		sendbuf[12] = bYkOn?0x82:0x81;
	}

	sendbuf[0] = 0x68;
	sendbuf[1] = 0x09;
	sendbuf[2] = 0x09;
	sendbuf[3] = 0x68;
	sendbuf[4] = fc;
	sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6] = 0x2d;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x08;
	sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10] = wAddr;
	sendbuf[11] = wAddr>>8;
	sendbuf[12] = bYkOn?0x81:0x80;
	sendbuf[13] = GetCheckSum(sendbuf);
	sendbuf[14] = 0x16;
	this->Send(sendbuf,15);
	ShowMessage("Remote control execute");
}
void  CIEC101::SendPreYK(ST_BYTE fc, ST_UINT wAddr, ST_BOOLEAN bYkOn)
{
    ST_BYTE sendbuf[256];
	if (fc==0x2d) //单点
	{
		sendbuf[12] = bYkOn?0x81:0x80;
	}
	else if (fc==0x2e)
	{
		sendbuf[12] = bYkOn?0x82:0x81;
	}

	sendbuf[0] = 0x68;
	sendbuf[1] = 0x09;
	sendbuf[2] = 0x09;
	sendbuf[3] = 0x68;
	sendbuf[4] = fc;
	sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[6] = 0x2d;
	sendbuf[7] = 0x01;
	sendbuf[8] = 0x06;
	sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[10] = wAddr;
	sendbuf[11] = wAddr>>8;
	sendbuf[12] = bYkOn?0x81:0x80;
	sendbuf[13] = GetCheckSum(sendbuf);
	sendbuf[14] = 0x16;
	this->Send(sendbuf,15);
	ShowMessage("Remote control execute");
}

void  CIEC101::SendTime(ST_BYTE fc)//
{
    struct tm now_tm;
    DateTime::localtime (time(0), now_tm);
    newday=now_tm.tm_mday;
    ST_BYTE sendbuf[256];
    sendbuf[0] = 0x68;
    sendbuf[1] = 0x0F;
    sendbuf[2] = 0x0F;
    sendbuf[3] = 0x68;
    sendbuf[4] = fc;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = 0x67;
    sendbuf[7] = 0x01;
    sendbuf[8] = 0x06;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10]= 0x00;
    sendbuf[11]= 0x00;
    sendbuf[12]= (now_tm.tm_sec * 1000)/256; //ms
	sendbuf[13]=now_tm.tm_min;  //min
	sendbuf[14]=now_tm.tm_hour; //hour
	sendbuf[15]=now_tm.tm_mday; //day
	sendbuf[16]=now_tm.tm_mon + 1;  //mon
	sendbuf[17]=now_tm.tm_year - 100;  //year
    sendbuf[18]= GetCheckSum(sendbuf);
    sendbuf[19]= 0x16;
    this->Send(sendbuf,20);
}

void CIEC101::Explain_M_SP_NA_1(ST_BYTE* pbuf,ST_INT len)  //单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i]&0x01;
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*3] + pbuf[4+i*3]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*3]&0x01;
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void CIEC101::Explain_M_SP_TA_1(ST_BYTE* pbuf,ST_INT len)  //带时标的单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i*4]&0x01;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[6+i*4]+pbuf[7+i*4]*256;
			ST_INT nMinute = pbuf[8+i*4]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			ST_INT nOutPoint = -1;
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName,nIndexYX,byValue?"ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*4]+pbuf[7+i*4]*256;
			ST_INT nMinute = pbuf[8+i*4]&0x3f;
			ST_INT nHour = pbuf[9+i*4]&0x3f;

			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX++,nHour,nMinute,nMillisecond);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*6] + pbuf[4+i*6]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*6]&0x01;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[6+i*6]+pbuf[7+i*6]*256;
			ST_INT nMinute = pbuf[8+i*6]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName,nIndexYX,byValue? "ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*4]+pbuf[7+i*4]*256;
			ST_INT nMinute = pbuf[8+i*4]&0x3f;
			ST_INT nHour = pbuf[9+i*4]&0x3f;

			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX,nHour,nMinute,nMillisecond);
		}
	}
}

void CIEC101::Explain_M_DP_NA_1(ST_BYTE* pbuf,ST_INT len)  //双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*3] + pbuf[4+i*3]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*3]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
		}
	}
}

void CIEC101::Explain_M_DP_TA_1(ST_BYTE* pbuf,ST_INT len)  //带时标的双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i*4]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[6+i*4]+pbuf[7+i*4]*256;
			ST_INT nMinute = pbuf[8+i*4]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			ST_INT nOutPoint = -1;
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName,nIndexYX,(byValue==1)?"ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/

			ST_INT nMillisecond = pbuf[6+i*6]+pbuf[7+i*6]*256;
			ST_INT nMinute = pbuf[8+i*6]&0x3f;
			ST_INT nHour = pbuf[9+i*6]&0x3f;

			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX++,nHour,nMinute,nMillisecond);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*6] + pbuf[4+i*6]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*6]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			ST_INT nMillisecond = pbuf[6+i*6]+pbuf[7+i*6]*256;
			ST_INT nMinute = pbuf[8+i*6]&0x3f;
			CTime tm = CTime::GetCurrentTime();
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
					tm.GetYear(),tm.GetMonth(),tm.GetDay(),tm.GetHour(),nMinute,nMillisecond/1000,nMillisecond%1000,
					chSoeName,nIndexYX,(byValue==1)? "ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/

			ST_INT nMillisecond = pbuf[6+i*6]+pbuf[7+i*6]*256;
			ST_INT nMinute = pbuf[8+i*6]&0x3f;
			ST_INT nHour = pbuf[9+i*6]&0x3f;
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX,nHour,nMinute,nMillisecond);
		}
	}
}

void CIEC101::Explain_M_SP_TB_1(ST_BYTE* pbuf,ST_INT len) //带CP56Time2a时标的单点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i*8]&0x01;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.msec  = pbuf[6+i*8] +pbuf[7+i*8]*256;
			st.min   = pbuf[8+i*8];
			st.hour  = pbuf[9+i*8];
			st.mday  = pbuf[10+i*8];
			st.month = pbuf[11+i*8];
			st.year  = pbuf[12+i*8];
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
				2000 + st.year, st.month, st.mday, st.hour, st.min, st.msec / 1000, st.msec % 1000,
					chSoeName,nIndexYX,byValue? "ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*8] +pbuf[7+i*8]*256;
			ST_INT nMinute = pbuf[8+i*8];
			ST_INT nHour = pbuf[9+i*8];
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX++,nHour,nMinute,nMillisecond);
		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*10] + pbuf[4+i*10]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*10]&0x01;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.msec  = pbuf[6 + i * 10] + pbuf[7 + i * 10] * 256;
			st.min   = pbuf[8+i*10];
			st.hour  = pbuf[9+i*10];
			st.mday  = pbuf[10+i*10];
			st.month = pbuf[11+i*10];
			st.year  = pbuf[12+i*10];
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
				2000 + st.year, st.month, st.mday, st.hour, st.min, st.msec / 1000, st.msec % 1000,
					chSoeName,byValue? "ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*8] +pbuf[7+i*8]*256;
			ST_INT nMinute = pbuf[8+i*8];
			ST_INT nHour = pbuf[9+i*8];
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX,nHour,nMinute,nMillisecond);
		}
	}
}

void CIEC101::Explain_M_DP_TB_1(ST_BYTE* pbuf,ST_INT len) //带CP56Time2a时标的双点信息
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		ST_INT nIndexYX = pbuf[3]+pbuf[4]*256 - 0x0001;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_BYTE byValue = pbuf[5+i*8]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.msec  = pbuf[6 + i * 8] + pbuf[7 + i * 8] * 256;
			st.min   = pbuf[8+i*8];
			st.hour  = pbuf[9+i*8];
			st.mday  = pbuf[10+i*8];
			st.month = pbuf[11+i*8];
			st.month = pbuf[12+i*8];
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
				2000 + st.year, st.month, st.mday, st.hour, st.min, st.msec / 1000, st.msec % 1000,
					chSoeName,nIndexYX,(byValue == 1)?"ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*8] +pbuf[7+i*8]*256;
			ST_INT nMinute = pbuf[8+i*8];
			ST_INT nHour = pbuf[9+i*8];
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX++,nHour,nMinute,nMillisecond);

		}
	}
	else
	{
		ST_INT nYXNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYXNum;i++)
		{
			ST_INT nIndexYX =  pbuf[3+i*10] + pbuf[4+i*10]*256 - 0x0001;
			ST_BYTE byValue =pbuf[5+i*10]&0x03;
			if(byValue == 0x02) byValue = 1;
			else if(byValue == 0x01) byValue = 0;
			else byValue = 2;
			/*
			/////SOE///////////////////
			CP56Time2a st;
			st.msec  = pbuf[6 + i * 10] + pbuf[7 + i * 10] * 256;
			st.min   = pbuf[8+i*10];
			st.hour  = pbuf[9+i*10];
			st.mday  = pbuf[10+i*10];
			st.month = pbuf[11+i*10];
			st.month = pbuf[12+i*10];
			char chDeviceName[64];
			char chSoeName[64];
			memcpy(chDeviceName,this->GetCurLine()->m_lineParam.lineName,64);
			sprintf(chSoeName,getVarname(chDeviceName,nIndexYX));
			CString soestr;
			soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s event %s ",
				2000 + st.year, st.month, st.mday, st.hour, st.min, st.msec / 1000, st.msec % 1000,
					chSoeName,nIndexYX,(byValue==1)?"ON":"OFF");
			this->ReportEvent(soestr,"soebj");
			///////////////////////////
			*/
			ST_INT nMillisecond = pbuf[6+i*8] +pbuf[7+i*8]*256;
			ST_INT nMinute = pbuf[8+i*8];
			ST_INT nHour = pbuf[9+i*8];
			this->UpdateValue(nIndexYX,(ST_BYTE)byValue);
			TransferEx(byValue,nIndexYX,nHour,nMinute,nMillisecond);
		}
	}
}

void CIEC101::Explain_M_ME_NA_1(ST_BYTE* pbuf,ST_INT len)  //测量值，规一化值
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;

		ST_INT nIndexYC = 0;
//		if(m_b0701==0x01)
//			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x0701 + 10000;
//		else if (m_b0701==0x02)
//		{
//			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x01f5 + 10000;
//		}
//		else
			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[6+i*3]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*3] - pbuf[6+i*3]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*3] + pbuf[6+i*3]*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
//			if(m_b0701==0x01)
//				nIndexYC = pbuf[3]+pbuf[4]*256 - 0x0701 + 10000;
//			else if (m_b0701==0x02)
//			{
//				nIndexYC = pbuf[3+i*3]+pbuf[4+i*3]*256 - 0x01f5 + 10000;
//			}
//			else
				nIndexYC = pbuf[3]+pbuf[4]*256 - 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[6+i*5]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*5] - pbuf[6+i*5]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*5] + (pbuf[6+i*5]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
		}
	}
}

void CIEC101::Explain_M_ME_NB_1(ST_BYTE* pbuf,ST_INT len) //测量值，标度化值
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
//		if(m_b0701)
//			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x0701 + 10000;
//		else
			nIndexYC =pbuf[3]+pbuf[4]*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[6+i*3]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*3] - pbuf[6+i*3]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*3] + pbuf[6+i*3]*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
//			if(m_b0701)
//				nIndexYC = pbuf[3+i*5] + pbuf[4+i*5]*256 - 0x0701 + 10000;
//			else
				nIndexYC =pbuf[3+i*5] + pbuf[4+i*5]*256 - 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[6+i*5]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*5] - pbuf[6+i*5]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*5] + (pbuf[6+i*5]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
		}
	}
}

void CIEC101::Explain_M_ME_NC_1(ST_BYTE* pbuf,ST_INT len) //测量值，短浮点数
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
//		if(m_b0701)
//			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x0701 + 10000;
//		else
			nIndexYC =pbuf[3]+pbuf[4]*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			DWORD value = pbuf[5+i*5] + pbuf[6+i*5]*256 + pbuf[7+i*5]*256*256 + pbuf[8+i*5]*256*256*256;
			float fValue = (float&)(*((short*)&value));
			this->UpdateValue(nIndexYC++,(float)fValue);
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
//			if(m_b0701)
//				nIndexYC = pbuf[3+i*7] + pbuf[4+i*7]*256 - 0x0701 + 10000;
//			else
				nIndexYC =pbuf[3+i*7] + pbuf[4+i*7]*256 - 0x4001 + 10000;
			DWORD value = pbuf[5+i*7] + pbuf[6+i*7]*256+ pbuf[7+i*7]*256*256 + pbuf[8+i*7]*256*256*256;
			float fValue = (float&)(*((short*)&value));
			this->UpdateValue(nIndexYC,(float)fValue);
		}
	}
}

void CIEC101::Explain_M_ME_ND_1(ST_BYTE* pbuf,ST_INT len) //测量值，不带品质描述词的规一化值
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = 0;
//		if(m_b0701)
//			nIndexYC = pbuf[3]+pbuf[4]*256 - 0x0701 + 10000;
//		else
			nIndexYC =pbuf[3]+pbuf[4]*256 - 0x4001 + 10000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_BOOLEAN bSign = pbuf[6+i*2]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*2] - pbuf[6+i*2]*256));
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*2] + (pbuf[6+i*2]&0x7f)*256;
				this->UpdateValue(nIndexYC++,(float)fValue);
			}
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC = 0;
//			if(m_b0701)
//				nIndexYC = pbuf[3+i*4] + pbuf[4+i*4]*256 - 0x0701 + 10000;
//			else
				nIndexYC =pbuf[3+i*4] + pbuf[4+i*4]*256 - 0x4001 + 10000;
			ST_BOOLEAN bSign = pbuf[6+i*4]&0x80;
			if(bSign)
			{
				float fValue = -1*((0xffff - pbuf[5+i*4] - pbuf[6+i*4]*256));
				this->UpdateValue(nIndexYC,(float)fValue);
			}
			else
			{
				float fValue = pbuf[5+i*4] + (pbuf[6+i*4]&0x7f)*256;
				this->UpdateValue(nIndexYC,(float)fValue);
			}
		}
	}
}

void CIEC101::Explain_M_IT_NA_1(ST_BYTE* pbuf,ST_INT len) //电能脉冲量
{
	if(pbuf[0]&0x80) //顺序
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		ST_INT nIndexYC = pbuf[3]+pbuf[4]*256 - 0x6401 + 20000;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			double dbValue = pbuf[5+i*5] + pbuf[6+i*5]*256 + pbuf[7+i*5]*256*256 + pbuf[8+i*5]*256*256*256;
			this->UpdateValue(nIndexYC++,(double)dbValue);
		}
	}
	else
	{
		ST_INT nYCNum = pbuf[0]&0x7f;
		for(ST_INT i=0;i<nYCNum;i++)
		{
			ST_INT nIndexYC =  pbuf[3+i*7] + pbuf[4+i*7]*256 - 0x6401 + 20000;
			double dbValue = pbuf[5+i*7] + pbuf[6+i*7]*256 + pbuf[7+i*7]*256*256 + pbuf[8+i*7]*256*256*256;
			this->UpdateValue(nIndexYC,(double)dbValue);
		}
	}
}

void    CIEC101::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
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
