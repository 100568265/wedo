#include "CInner.h"
#include <unistd.h>
#include "EngineBase.h"
#include <math.h>
#include "Channel.h"

#include "Debug.h"

#define sDebug  if (true) wedoDebug (SysLogger::GetInstance()).noquote

extern NodeTree *g_pTree;



CInner::CInner()
:is_initial(true)
{
    //ctor
}

CInner::~CInner()
{
    //dtor
}

void    CInner::Init()
{
	nsendfailnum = 0;
    for(ST_INT ib = 0; ib < 1024; ++ib)
    {
        m_Begin[ib] = true;
    }
    m_bTask      = false;
    is_totalcall = false;
}

void	CInner::Uninit()
{

}

void	CInner::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    if(this->HasTask() && this->GetTask(&m_curTask))
    {
        if     (!strcmp(m_curTask.taskCmd,"SOE"))
        {
            SendSOE(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
            SendYKEcho(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"singlewrite"))
        {
            SendYTEcho(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"user_by_binary"))
        {
            ST_BYTE buf[32] = {0};
            buf[0] = 0xaa;
            buf[1] = 0xbb;
            buf[2] = 0x40;
            buf[3] = 0x03;
            buf[4] = 0x00;
            buf[5] =  m_curTask.transDeviceId&0x00ff;   //deviceid
            buf[6] = (m_curTask.transDeviceId&0xff00)>>8;
            buf[7] =  m_curTask.taskResult.resultCode;
            this->Send (&buf[0], 8);
            Memset(&m_curTask, 0, sizeof(m_curTask));
        }
        else if(!strcmp(m_curTask.taskCmd,"multiwrite"))
        {
            SendYKEcho(m_curTask);
        }
    }

	readed = 0;
	if(this->GetCurPort())
	{
		ST_INT	len = this->GetCurPort()->PickBytes(pbuf, 5, 300);
		if (len >= 5)
		{
			ST_INT star = 0;
			for(star = 0; star < len; star++)
			{
				if(pbuf[star] == 0xaa)
					break;
			}
			if(star > 0)
			{
				this->GetCurPort()->ReadBytes(pbuf,star);
				return;
			}
            if((pbuf[0] == 0xaa) && (pbuf[1] == 0xbb))
            {
                int ndatalen = pbuf[3] + pbuf[4] * 256 + 5;
                len = this->GetCurPort()->PickBytes(pbuf, ndatalen, 1000);
                if(len >= ndatalen)
                {
                    len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
                    readed = len;
                }
                else
                {
                    this->GetCurPort()->Clear();
                }
            }
            else
            {
                this->GetCurPort()->Clear();
            }
		}
		else
		{
			this->GetCurPort()->Clear();
		}
	}
    return;
}

ST_BOOLEAN	CInner::OnSend()
{
//	m_bTask = false;
	if(!this->IsOpened())
	{
	    for(ST_INT ib = 0; ib < 1024; ib++)
	    {
            m_Begin[ib] = true;
	    }
	    return false;
	}
    else if (is_initial) {
        is_initial = false;
        Thread::SLEEP(1000);
        ST_BYTE data[] = { 0xAA, 0xBB, 0x2B, 0x00, 0x00 };
        this->Send(data, sizeof(data));
        return true;
    }

    SendData();
    return true;
}

ST_BOOLEAN	CInner::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    if(len < 5) return false;

    switch(pbuf[2]) {
        case 0x07: {
            ST_BYTE data[] = {0xAA, 0xBB, 0x0B, 0x00, 0x00};
            this->Send (data, sizeof(data));
            Thread::SLEEP (20);
        } break;
        case 0x10: {
            switch (pbuf[7]) {
                case 0x01: { // YK
                    ProtocolTask task;
                    task.isTransfer     = true;
                    task.transChannelId = -1;
                    task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
                    Strcpy(task.taskCmd, "devicecontrol");
                    task.taskCmdCode = pbuf[8];
                    task.taskAddr    = pbuf[9] + pbuf[10] * 256;
                    task.taskValue   = pbuf[11];
                    Transfer(&task);
                } break;
                case 0x02: { // YT
                    ProtocolTask task;
                    task.isTransfer     = true;
                    task.transChannelId = -1;
                    task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
                    strcpy(task.taskCmd, "singlewrite");
                    task.taskCmdCode = pbuf[8];
                    task.taskAddr    = pbuf[9] + pbuf[10] * 256;
                    float value = 0;
                    memcpy(&value, pbuf + 11, sizeof(value));
                    task.taskValue = value;
                    Transfer(&task);
                } break;
            }
        } break;
        case 0x20: {
            size_t plen = pbuf[3] + pbuf[4] * 256 - 2;
            if (plen <= 0) break;

            ProtocolTask task;
            task.isTransfer     = true;
            task.transChannelId = -1;
            task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
            Strcpy(task.taskCmd, "user_by_binary");
            task.taskParamLen = plen;
            memcpy(task.taskParam, pbuf + 7, plen);
            Transfer(&task);
        } break;
        case 0x43: {
            ST_BYTE data[] = {0xAA, 0xBB, 0x83, 0x00, 0x00};
            this->Send (data, sizeof(data));
            Thread::SLEEP (20);
        } break;
        case 0x83: {

        } break;
        case 0xFF: {
            ST_BYTE data[] = {0xAA, 0xBB, 0xFF, 0x00, 0x00};
            this->Send (data, sizeof(data));
            Thread::SLEEP (20);
            is_totalcall = true;
        } break;
        case 0x88: {
            ProtocolTask task;
            task.isTransfer     = true;
            task.transChannelId = -1;
            task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
            Strcpy(task.taskCmd, "recharge");
            task.taskCmdCode = pbuf[8];
            task.taskAddr = pbuf[9] + pbuf[10] * 256;
            task.taskAddr1 = pbuf[11];
            task.timeOut = (pbuf[12] << 24) | (pbuf[13] << 16) | (pbuf[14] << 8) | pbuf[15];

            Transfer(&task);
            m_pLogger->LogDebug("parameters in protocol-inner:");
            //m_pLogger->LogDebug("pbuf[12]=%d",pbuf[12]);
            m_pLogger->LogDebug("task.timeOut=%d",task.timeOut);
            m_pLogger->LogDebug("------------------------------");
            //充值5秒後更新電表數據
            Thread::SLEEP(1000*5);
            SendData();
        }break;
        default:
            break;
    }
    return true;
}

ST_BOOLEAN	CInner::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

CInner* CreateInstace()
{
   return new  CInner();
}

void  CInner::SendSOE(ProtocolTask& curTask)
{
//	if(curTask.taskCmdCode == 0)
	{
        ST_BYTE data[64] = {0};
        data[0] = 0xaa;
        data[1] = 0xbb;
        data[2] = 0x02;
        data[3] = 0x0e;
        data[4] = 0x00;
        data[5] = curTask.taskParam[12];   //deviceid
        data[6] = curTask.taskParam[13];
        data[7] = curTask.taskParam[10];     //point
        data[8] = curTask.taskParam[11];
        data[9] = curTask.taskParam[9];//statu
        data[10] = curTask.taskParam[0]; //year_L
        data[11] = curTask.taskParam[1]; //year_H
        data[12] = curTask.taskParam[2]; //month
        data[13] = curTask.taskParam[3]; //day
        data[14] = curTask.taskParam[4]; //hour
        data[15] = curTask.taskParam[5]; //minute
        data[16] = curTask.taskParam[6]; //second
        data[17] = curTask.taskParam[7]; //msecond_L
        data[18] = curTask.taskParam[8]; //msecond_H
        this->Send(data, 19);
        Memset(&curTask, 0, sizeof(curTask));
	}
}

void    CInner::SendYKEcho(ProtocolTask& curTask)
{
 	if((curTask.taskCmdCode == 0) || (curTask.taskCmdCode == 1) || (curTask.taskCmdCode == 2))
	{
        ST_BYTE bySendbuffer[64];
        bySendbuffer[0] = 0xaa;
        bySendbuffer[1] = 0xbb;
        bySendbuffer[2] = 0x80;
        bySendbuffer[3] = 0x03;
        bySendbuffer[4] = 0x00;
        bySendbuffer[5] = curTask.transDeviceId&0xff;   //deviceid
        bySendbuffer[6] = (curTask.transDeviceId&0xff00)>>8;
        bySendbuffer[7] = curTask.taskResult.resultCode;
//        m_pLogger->LogDebug("curTask.taskResult.resultCode%d",curTask.taskResult.resultCode);
        this->Send(&bySendbuffer[0],8);
        Memset(&curTask, 0, sizeof(curTask));
	}
}

void CInner::SendYTEcho (ProtocolTask & _task)
{
    ST_BYTE data[32] = { 0xAA, 0xBB, 0x00 };
    data[2] = 0x80;
    data[3] = 0x03;
    data[4] = 0x00;
    data[5] = _task.transDeviceId % 256;
    data[6] = _task.transDeviceId / 256;
    data[7] = _task.taskResult.resultCode;

    this->Send(&data[0], 8);
    Memset(&_task, 0, sizeof(_task));
}

void CInner::SendData()
{
    ST_INT interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
    interval = interval > 30 ? interval : 30;

    ST_INT j = 0;
    ST_INT  nDataLen = 0;
	ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
	for(ST_INT i = 0; i < devcount; ++i)
	{
		if (g_pTree->GetNodeName("", i, devname) < 0)
            continue;

		ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        if (varcount <= 0) continue;

		for(j = 0; j < varcount; ++j)
		{
//            memset(fullname, 0, sizeof(fullname) / 2);
//            memset(ditname , 0, sizeof(ditname ) / 2);
            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

		    g_pTree->GetNodeName(devname, j, ditname);

            strcpy(fullname, devname); strcat(fullname, ".");
		    strcat(fullname, ditname); strcat(fullname, ".value");
		    ST_DUADDR tdd;
		    g_pTree->GetNodeAddr(fullname, tdd);
            ST_VARIANT vValue;
            // GetVariableValueByName(&fullname[0],vValue);
            GetVariableValueByAddr(tdd, vValue);

            if(tdd.type == -2) continue;
            bySendbuf[0] = 0xaa;
            bySendbuf[1] = 0xbb;
            bySendbuf[2] = 0x01;
            bySendbuf[5] =  tdd.connect& 0xff;
            bySendbuf[6] = (tdd.connect& 0xff00)>>8;
            bySendbuf[7] =  tdd.device & 0xff;
			bySendbuf[8] = (tdd.device & 0xff00)>>8;
			bySendbuf[9+nDataLen]  =  tdd.addr & 0xff;
            bySendbuf[10+nDataLen] = (tdd.addr & 0xff00)>>8;
			bySendbuf[11+nDataLen] = vValue.vt;
            bySendbuf[12+nDataLen] = tdd.type;
            if(tdd.type == -1)
            {
                bySendbuf[2] = 0x03;
                bySendbuf[12+nDataLen] = 254;
            }
            switch(vValue.vt)
            {
            case VALType_SByte:
                {
                    ST_BYTE Value = vValue.bVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        bySendbuf[13+nDataLen] = Value;
                        nDataLen = nDataLen+5;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Byte:
                {
                    ST_BYTE Value = vValue.bVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        bySendbuf[13+nDataLen] = Value;
                        nDataLen = nDataLen+5;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Boolean:
                {
                    ST_BYTE Value = vValue.blVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        bySendbuf[13+nDataLen] = Value;
                        nDataLen = nDataLen+5;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Int16:
                {
                    ST_INT Value  = vValue.sVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,2);
                        nDataLen = nDataLen+6;
                        preValue[i][j] = Value;
                   }
                }
                break;
            case VALType_UInt16:
                {
                    ST_INT Value  = vValue.usVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,2);
                        nDataLen = nDataLen+6;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Int32:
                {
                    ST_INT Value  = vValue.iVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,4);
                        nDataLen = nDataLen+8;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_UInt32:
                {
                    ST_INT Value  = vValue.uiVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,4);
                        nDataLen = nDataLen+8;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Float:
                {
                    ST_FLOAT Value  = vValue.fVal;
                    ST_BOOLEAN bchange = true;
                    if(preValue[i][j] != 0.0)
                    {
                        ST_FLOAT f1 = fabs (preValue[i][j] - Value);
                        //if((f1 / fabs (preValue[i][j])) < 0.01)
                            //bchange = false;
                        if(f1 > 1 || Value == 0.0)  // 380 * 0.01 = 3.8
                            bchange = true;
                        /*if (f1 < 0.1)
                            bchange = false;
                        if(Value == 0.0)
                            bchange = true;*/
                    }
                    if(((preValue[i][j] != Value) && bchange) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13 + nDataLen], &Value, 4);
                        nDataLen = nDataLen + 8;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Int64:
                {
                    ST_LONG Value  = vValue.lVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,8);
                        nDataLen = nDataLen+12;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_UInt64:
                {
                    ST_LONG Value  = vValue.ulVal;
                    if((preValue[i][j] != Value) || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13+nDataLen],&Value,8);
                        nDataLen = nDataLen+12;
                        preValue[i][j] = Value;
                    }
                }
                break;
            case VALType_Double:
                {
                    ST_DOUBLE Value  = vValue.dtVal;
                    if(fabs (preValue[i][j] - Value) > 0.001 || !m_Begin[i])
                    {
                        memcpy(&bySendbuf[13 + nDataLen], &Value, 8);
                        nDataLen = nDataLen + 12;
                        preValue[i][j] = Value;
                    }
                }
                break;
            default:
                break;
            }
            if (nDataLen >= 300) {
				nDataLen += 4;
				bySendbuf[3] =  nDataLen & 0xff;
				bySendbuf[4] = (nDataLen & 0xff00) >> 8;
				this->Send(bySendbuf, nDataLen + 5);
				Thread::SLEEP(interval);
                nDataLen = 0;
			}
        }
        if (nDataLen > 0)
        {
            nDataLen += 4;
            bySendbuf[3] =  nDataLen & 0xff;
            bySendbuf[4] = (nDataLen & 0xff00) >> 8;
            ST_INT nr = this->Send(bySendbuf, nDataLen + 5);
            Thread::SLEEP(interval);

            if(nr < 1) {
                if(nsendfailnum++ > 20) {
                    nsendfailnum = 0;
                    this->Close();
                    Thread::SLEEP(1000);
                    this->Open();
                }
            }
            else
                nsendfailnum = 0;
        }
        nDataLen   = 0;
		m_Begin[i] = true;

		if (is_totalcall) {
            is_totalcall = false;
            memset (m_Begin, 0, sizeof(m_Begin));
            return;
		}
	}
}
