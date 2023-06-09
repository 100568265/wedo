#include "CInner.h"
#include <unistd.h>
#include "EngineBase.h"
#include <math.h>
#include "Channel.h"

#include "Debug.h"

#define sDebug  if (true) wedoDebug (SysLogger::GetInstance()).noquote

extern NodeTree *g_pTree;
static const uint16_t crc16_table[256] = {
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
	0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
	0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
	0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
	0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
	0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
	0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
	0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
	0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
	0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
	0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
	0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
	0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
	0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
	0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
	0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
	0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
	0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
	0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
	0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
	0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
	0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
	0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
	0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
	0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
	0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
	0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
	0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

inline void set_crc16_low (uint8_t *pdata, int nsize)
{
	if (nsize < 0) return;
	uint16_t crc_value = get_crc16 (pdata, nsize);
	memcpy (pdata + nsize, &crc_value, sizeof(crc_value));
}

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
                int ndatalen = pbuf[3] + pbuf[4] * 256 + 5 +2;//2：CRC check
                len = this->GetCurPort()->PickBytes(pbuf, ndatalen, 1000);
                if(len >= ndatalen)
                {
                    len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
                    if (pbuf[ndatalen - 2] + pbuf[ndatalen - 1] * 256 != get_crc16(pbuf, ndatalen - 2))//lxb  添加CRC校验
                    {
                        ShowMessage ("Check error!");
                        this->GetCurPort()->Clear();
                        return;
                    }
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
        ST_BYTE data[] = { 0xAA, 0xBB, 0x2B, 0x00, 0x00,0x68,0xF4 };
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
            ST_BYTE data[] = {0xAA, 0xBB, 0x0B, 0x00, 0x00,0x69,0x3E};
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
            ST_BYTE data[] = {0xAA, 0xBB, 0x83, 0x00, 0x00,0xE9,0x14};
            this->Send (data, sizeof(data));
            Thread::SLEEP (20);
        } break;
        case 0x83: {

        } break;
        case 0xFF: {
            ST_BYTE data[] = {0xAA, 0xBB, 0xFF, 0x00, 0x00,0x28,0xCC};
            this->Send (data, sizeof(data));
            Thread::SLEEP (20);
            is_totalcall = true;
        } break;
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
        set_crc16_low (data, 19);
        this->Send(data, 21);
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
        set_crc16_low(bySendbuffer,8);
        this->Send(&bySendbuffer[0],10);
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
    set_crc16_low(data,8);
    this->Send(&data[0], 10);
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
                        if((f1 / fabs (preValue[i][j])) < 0.02)
                            bchange = false;
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
                    if(fabs (preValue[i][j] - Value) > 0.02 || !m_Begin[i])
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
				set_crc16_low(bySendbuf,nDataLen + 5);
				this->Send(bySendbuf, nDataLen + 7);//5+2(CRC校验)
				Thread::SLEEP(interval);
                nDataLen = 0;
			}
        }
        if (nDataLen > 0)
        {
            nDataLen += 4;
            bySendbuf[3] =  nDataLen & 0xff;
            bySendbuf[4] = (nDataLen & 0xff00) >> 8;
            set_crc16_low(bySendbuf,nDataLen + 5);
            ST_INT nr = this->Send(bySendbuf, nDataLen + 7);//5+2(CRC校验)
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
