// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "CDL645.h"
#include "Device.h"
#include "stdio.h"

static const ST_CHAR* const STR_TYPE[] = {
	"单相电表",
	"三相电表"
};
enum SY_Model {
	SINGLE = 0x00,
	THREEPHASE = 0x01,
	XLast
};

inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
	ST_BYTE bySum = 0x00;
	for(int i = 0; i < len; ++i)
	{
		bySum += pbuf[i];
	}
	return  bySum;
}

Protocol * CreateInstace()
{
	return new CDL645();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDL645::CDL645()
{
    m_readIndex = 0;
	memset (m_addrarea, 0, sizeof(m_addrarea));
	memset (&m_curTask,0,sizeof(m_curTask));
	isYK = false;

    modelType     = XLast;

}

CDL645::~CDL645()
{
}

void CDL645::Init()
{
    if (!this->GetDevice()){
       printf("GetDevice is NULL\r\n");
       return;
    }

    string name   = this->GetDevice()->GetDeviceInfo()->Deviceserialtype;
    //printf("name : %s\r\n",name.c_str());
    for (ST_UINT i = 0; i < XLast; ++i)
    {
        //printf("STR_TYPE : %s\r\n",STR_TYPE[i]);
    	if (name == STR_TYPE[i])
    		modelType = i;
    }
}

void CDL645::Uninit()
{
}

bool CDL645::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void CDL645::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
		if(len < 12) {
			ShowMessage ("Insufficient data length");
			return;
		}
		ST_INT star = 0;
		for(; star < len; ++star) {
			if(pbuf[star] == 0x68)
				break;
		}
		if(len == star) {
			ShowMessage ("Garbled code, clear buffer.");
			this->GetCurPort()->Clear();
			return;
		}
		if(star > 0)
		{
			this->GetCurPort()->ReadBytes(pbuf, star);
		}

        if (this->GetDevice()->GetDeviceInfo()->DataAreasCount <= 0)
        {
            ShowMessage("No Point Table.");
            this->GetCurPort()->Clear();
            return;
        }

		len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
		if((pbuf[0] == 0x68) && (pbuf[7] == 0x68))
		{
			ST_INT ndatalen = pbuf[9] + 12;
			if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) == ndatalen)
			{
                if (memcmp(pbuf + 1, m_addrarea, sizeof(m_addrarea)))
				{
					this->ShowMessage("Address not match.");
					return;
				}
				if (get_check_sum(pbuf, ndatalen - 2) == pbuf[ndatalen - 2])
				{
					readed = ndatalen;
					return;
				}
				else {
					ShowMessage("Check error!");
					this->ShowRecvFrame(pbuf,len);
					this->GetCurPort()->Clear();
					return;
				}
			}
		}
		else
		{
			ShowMessage("Sync header error.");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
			return;
		}
	}
	else
        isYK = false;
}

bool CDL645::OnSend()
{
    if (this->HasTask() && this->GetTask(&m_curTask))
    {
        if (!strcmp (m_curTask.taskCmd, "devicecontrol"))
        {
            if (m_curTask.taskCmdCode == 1)
            {
                SendYKExec((bool)m_curTask.taskValue);
                isYK = true;
            }
            return true;
        }
    }

	switch(m_readIndex) {
		case 0:  ReadData(0x00000000); break; // 当前组合有功总电能
		case 1:  ReadData(0x0201ff00); break; // 电压
		case 2: ReadData(0x0202ff00); break; // 电流
		case 3: ReadData(0x0203ff00); break; // 有功功率
		case 4: ReadData(0x0206ff00); break; // 功率因数
		case 5: ReadData(0x04000503); break; // 状态字3
 		default: break;
	}
	m_readIndex = ((++m_readIndex) % 6); // 19+1
	return true;
}

bool CDL645::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
    if (isYK)
    {
        m_curTask.taskResult.resultCode = (pbuf[8] & 0x40)? 1:0;
        m_curTask.isTransfer = 1;
        Transfer(&m_curTask);
		memset(&m_curTask,0,sizeof(m_curTask));
		isYK = false;
		m_readIndex = 5;
		return true;
    }

    switch(modelType)
    {
    case SINGLE:
        singleAnalaze(pbuf,len);
        break;
    case THREEPHASE:
        threeAnalaze(pbuf,len);
        break;
    case XLast:
        ShowMessage("XLast modeType,noe Anaylze!");
        break;
    default:
        ShowMessage("not found modeType");
        break;
    }

	return true;
}

void    CDL645::ReadData(ST_UINT32 wAddr) //参数由高到低，写出由低到高  主站请求帧
{
	ST_BYTE sendbuf[32] = {0};
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[0] = 0x68;

	m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
	m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
	m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 01, m_addrarea, sizeof(m_addrarea));

	sendbuf[7] = 0x68;
	sendbuf[8] = 0x11;
	sendbuf[9] = 0x04;

	ST_UINT16 wTempH = (wAddr & 0xffff0000) >> 16;
	ST_UINT16 wTempL =  wAddr & 0x0000ffff;
	sendbuf[10] = ( wTempL&0x00ff)    +0x33;
	sendbuf[11] = ((wTempL&0xff00)>>8)+0x33;
	sendbuf[12] = ( wTempH&0x00ff)    +0x33;
	sendbuf[13] = ((wTempH&0xff00)>>8)+0x33;

    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 14; ++i)
	{
		bySum += sendbuf[i];
	}
	sendbuf[14] = bySum;
	sendbuf[15] = 0x16;
	this->Send(sendbuf, 16);
}


void    CDL645::SendYKExec(bool YKBit)
{
	time_t stamp=time(NULL)+24*60*60;
	struct tm *t=localtime(&stamp);

	ST_BYTE sendbuf[256] = {0};
	sendbuf[0]	= 0x68;

	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
	m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
	m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
	m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
	m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
	m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 01, m_addrarea, sizeof(m_addrarea));


	sendbuf[7]	= 0x68;
	sendbuf[8]	= 0x1C;
	sendbuf[9]	= 0x10;;
	// PA - P2
	sendbuf[10] = 0x35;
	sendbuf[11] = 0x89;
	sendbuf[12] = 0x67;
	sendbuf[13] = 0x45;
	// C0-C3
	sendbuf[14] = 0xB8;
	sendbuf[15] = 0x73;
	sendbuf[16] = 0x39;
	sendbuf[17] = 0xA3;
	// 数据N-Nm
	sendbuf[18] = YKBit ? 0x4F : 0x4D; // 如果是集中器就是 4E  试试才知道。
	sendbuf[19] = 0x33;
	//  ssmmhhDDMMYY + 33
	sendbuf[20] = 0x8C;
	sendbuf[21] = 0x8C;
	sendbuf[22] = 0x56;
	sendbuf[23] = DecToBCD(t->tm_mday)+0x33;
	sendbuf[24] = DecToBCD(t->tm_mon+1)+0x33;
	sendbuf[25] = DecToBCD((t->tm_year+1900)%100)+0x33;

	sendbuf[26] = CSCheck(sendbuf,26);
	sendbuf[27] = 0x16;
	this->Send(sendbuf,28);

}

ST_BYTE CDL645::DecToBCD(ST_BYTE num)
{
	ST_BYTE a, b, bcd;

	a = (num % 10) & 0x0f;
	b = ((num / 10) << 4) & 0xf0;
	bcd = a | b;

	return bcd;
}


ST_BYTE CDL645::CSCheck(ST_BYTE *buf,int len)
{
	ST_BYTE result = 0;
	for (int i=0; i<len; i++)
	{
		result += buf[i];
	}
	return result;
}

void    CDL645::singleAnalaze(ST_BYTE* pbuf,ST_INT len)
{
    if(pbuf[8] == 0x91) {
        unsigned long datatype = 0;
        memcpy(&datatype, pbuf + 10, sizeof(datatype));
        switch (datatype) {
            case 0x00000000:
            case 0x33333333:{
                //const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[0];
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0x70) >> 4) * 10000000)*0.01;

                    if ( (pbuf[17]-0x33) & 0x80 )
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                    }
                //if (itemref.coeficient != 0)
                //    fvalue = fvalue * itemref.coeficient;
                this->UpdateValue(0, fvalue);
            }break;


            case 0x0201FF00:
            case 0x35343233: {
                float fvalue = 0;
                //for(int i = 0; i < 3; ++i)
                //{
                    fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xF0)>>4)*1000)*0.1;
                    this->UpdateValue(1, fvalue);
                //}
            } break;

            case 0x0202FF00:
            case 0x35353233: {
                float fvalue = 0;
                //for(int i = 0; i < 3; ++i)
                //{
                    fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0x70)>>4)*100000)*0.001;

                    if ((pbuf[16]-0x33) & 0x80)
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                    }
                    this->UpdateValue(2, fvalue);
                //}
            } break;

            case 0x0203FF00:
            case 0x35363233: {
                float fvalue = 0;
                //for(int i = 0; i < 4; ++i)
                //{
                    fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0x70)>>4)*100000)*0.0001;

                    if ((pbuf[16] -0x33) & 0x80)
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                    }
                    this->UpdateValue(3, fvalue);
                //}
            } break;


            case 0x0206FF00:
            case 0x35393233: {
                float fvalue = 0;
                //for (int i = 0; i < 4; ++i)
                //{
                    fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0x70)>>4)*1000)*0.001;

                    if ((pbuf[15]-0x33) & 0x80)
                    {
                        if (fvalue > 1)
                            fvalue = 1.000;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 1)
                            fvalue = 1.000;
                    }
                    this->UpdateValue(4, fvalue);
                //}
            } break;

            case 0x04000503:
            case 0x37333836: {
			ST_BYTE bith = pbuf[15] - 0x33;
			ST_BYTE bitl = pbuf[14] - 0x33;
			ST_BYTE value = 0;
			value = (bitl & 0x10)? 1:0;
			this->UpdateValue(5, value?0:1);

			} break;

            default: {
                this->ShowMessage("UnKown data id.");
            } break;
        }
	}
	else if (pbuf[8] == 0xD1 && pbuf[9] == 0x01)
	{
	    switch(pbuf[10]-0x33)
	    {
	        case 0x01: this->ShowMessage("Device error response，the reason：other."); break;
	        case 0x02: this->ShowMessage("Device error response，the reason：no request data."); break;
	        case 0x04: this->ShowMessage("Device error response，the reason：no access."); break;
	        case 0x08: this->ShowMessage("Device error response，the reason：baudrate can't change."); break;
	        case 0x10: this->ShowMessage("Device error response，the reason：year time zone overflow."); break;
	        case 0x20: this->ShowMessage("Device error response，the reason：day tiem zone overflow."); break;
	        case 0x40: this->ShowMessage("Device error response，the reason：rate overflow."); break;
	        default: this->ShowMessage("Device error response，the reason：undefined."); break;
	    }
	}
}
void    CDL645::threeAnalaze(ST_BYTE* pbuf,ST_INT len)
{
    if(pbuf[8] == 0x91) {
        unsigned long datatype = 0;
        memcpy(&datatype, pbuf + 10, sizeof(datatype));
        switch (datatype) {
            case 0x00000000:
            case 0x33333333:{
                //const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[0];
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0x70) >> 4) * 10000000)*0.01;

                    if ( (pbuf[17]-0x33) & 0x80 )
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                            fvalue = 799999.99;
                    }
                //if (itemref.coeficient != 0)
                //    fvalue = fvalue * itemref.coeficient;
                this->UpdateValue(0, fvalue);
            }break;


            case 0x0201FF00:
            case 0x35343233: {
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0xF0)>>4)*1000)*0.1;
                    this->UpdateValue(i +  1, fvalue);
                }
            } break;

            case 0x0202FF00:
            case 0x35353233: {
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.001;

                    if ((pbuf[16+i*3]-0x33) & 0x80)
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 799.999)
                            fvalue = 799.999;
                    }
                    this->UpdateValue(i +  4, fvalue);
                }
            } break;

            case 0x0203FF00:
            case 0x35363233: {
                float fvalue = 0;
                for(int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;

                    if ((pbuf[16+i*3] -0x33) & 0x80)
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 79.9999)
                            fvalue = 79.9999;
                    }
                    this->UpdateValue(i +  7, fvalue);
                }
            } break;


            case 0x0206FF00:
            case 0x35393233: {
                float fvalue = 0;
                for (int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0x70)>>4)*1000)*0.001;

                    if ((pbuf[15+i*2]-0x33) & 0x80)
                    {
                        if (fvalue > 1)
                            fvalue = 1.000;
                        fvalue = -1.0 * fvalue;
                    }
                    else
                    {
                        if (fvalue > 1)
                            fvalue = 1.000;
                    }
                    this->UpdateValue(i + 11, fvalue);
                }
            } break;

            case 0x04000503:
            case 0x37333836: {
			ST_BYTE bith = pbuf[15] - 0x33;
			ST_BYTE bitl = pbuf[14] - 0x33;
			ST_BYTE value = 0;
			value = (bitl & 0x10)? 1:0;
			this->UpdateValue(15, value?0:1);

			} break;

            default: {
                this->ShowMessage("UnKown data id.");
            } break;
        }
	}
	else if (pbuf[8] == 0xD1 && pbuf[9] == 0x01)
	{
	    switch(pbuf[10]-0x33)
	    {
	        case 0x01: this->ShowMessage("Device error response，the reason：other."); break;
	        case 0x02: this->ShowMessage("Device error response，the reason：no request data."); break;
	        case 0x04: this->ShowMessage("Device error response，the reason：no access."); break;
	        case 0x08: this->ShowMessage("Device error response，the reason：baudrate can't change."); break;
	        case 0x10: this->ShowMessage("Device error response，the reason：year time zone overflow."); break;
	        case 0x20: this->ShowMessage("Device error response，the reason：day tiem zone overflow."); break;
	        case 0x40: this->ShowMessage("Device error response，the reason：rate overflow."); break;
	        default: this->ShowMessage("Device error response，the reason：undefined."); break;
	    }
	}
}












