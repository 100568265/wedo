#include "DL645.h"
#include "Device.h"
#include "Channel.h"
#include "stdio.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

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
}

CDL645::~CDL645()
{

}

void CDL645::Init()
{}

void CDL645::Uninit()
{}

ST_BOOLEAN  CDL645::IsSupportEngine (ST_INT IsSupportEngine)
{
	return true;
}

inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
	ST_BYTE bySum = 0x00;
	for(int i = 0; i < len; ++i)
	{
		bySum += pbuf[i];
	}
	return  bySum;
}

void CDL645::OnRead(ST_BYTE* pbuf,int& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		int lineInterval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
		if (lineInterval < 3000)
			lineInterval = 3000;
		int	len = this->GetCurPort()->PickBytes(pbuf, 12, lineInterval);
		if(len >= 12)
		{
			int star = 0;
			for(; star < len; ++star)
			{
				if(pbuf[star] == 0x68)
					break;
			}
			if(len == star) {
				this->GetCurPort()->Clear();
				return;
			}
			if(star > 0) {
			//	this->ShowRecvFrame(pbuf,len);
				this->GetCurPort()->ReadBytes(pbuf, star);
			}
			//ReadIndex = star;
			len = this->GetCurPort()->PickBytes(pbuf, 12, lineInterval);
			if((pbuf[0] == 0x68) && (pbuf[7] == 0x68) && pbuf[1]==sendbuf[1] && pbuf[2]==sendbuf[2] && pbuf[3]==sendbuf[3]
				&& pbuf[4]==sendbuf[4] && pbuf[5]==sendbuf[5] && pbuf[6]==sendbuf[6])
			{
				int ndatalen = pbuf[9] + 12;
				if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) < ndatalen)
					return;

				if (get_check_sum(pbuf,ndatalen - 2) == pbuf[ndatalen - 2])
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
			else {
				ShowMessage("Address error!");
				this->ShowRecvFrame(pbuf, len);
				this->GetCurPort()->Clear();
				return;
			}
		}
	}
}

bool CDL645::OnSend()
{

	switch(m_readIndex) {
        //case 0:  ReadData(0xC032); break; // 表号
        case 0:  ReadData(0X9010); break; // 当前正向有功总电能
        case 1:  ReadData(0x9020); break; // 当前反向有功总电能
        case 2:  ReadData(0x9110); break; // 正向无功总电能
        case 3:  ReadData(0x9120); break; // 反向无功总电能
        case 4:  ReadData(0x9130); break; // 一象限无功总电能
        case 5:  ReadData(0x9140); break; // 四象限无功总电能
        case 6:  ReadData(0x9150); break; // 二象限无功总电能
        case 7:  ReadData(0x9160); break; // 三象限无功总电能
        case 8:  ReadData(0xA010); break; // 当前正向有功最大需量
        //case 9:  ReadData(0xB010); break; // 当前正向有功最大需量时间
        case 9: ReadData(0xB611); break; // 当前A相电压
        case 10: ReadData(0xB612); break; // 当前B相电压
        case 11: ReadData(0xB613); break; // 当前C相电压
        case 12: ReadData(0xB621); break; // 当前A相电流
        case 13: ReadData(0xB622); break; // 当前B相电流
        case 14: ReadData(0xB623); break; // 当前C相电流
        case 15: ReadData(0xB630); break; // 当前有功功率
        case 16: ReadData(0xB631); break; // 当前A相有功功率
        case 17: ReadData(0xB632); break; // 当前B相有功功率
        case 18: ReadData(0xB633); break; // 当前C相有功功率
        case 19: ReadData(0xB640); break; // 当前无功功率
        case 20: ReadData(0XB641); break; // 当前A相无功功率
        case 21: ReadData(0xB642); break; // 当前B相无功功率
        case 22: ReadData(0xB643); break; // 当前C相无功功率
        case 23: ReadData(0xB650); break; // 总功率因数
        case 24: ReadData(0xB651); break; // A相功率因数
        case 25: ReadData(0xB652); break; // B相功率因数
        case 26: ReadData(0xB653); break; // C相功率因数
        case 27: ReadData(0xC020); break; // 电表运行状态字1
        case 28: ReadData(0xC021); break; // 电表运行状态字2
        case 29: ReadData(0xC022); break; // 电表运行状态字3

        case 30: ReadData(0x9011); break; // 费率1正向有功电能
        case 31: ReadData(0x9012); break; // 费率2正向有功电能
        case 32: ReadData(0x9013); break; // 费率3正向有功电能
        case 33: ReadData(0x9014); break; // 费率4正向有功电能
        default:
			break;
	}
	m_readIndex = (++m_readIndex % 34); // 29
	return true;
}

inline uint8_t  bcd2bin8  (uint8_t  value)
{
	return (value >> 4) * 10 + (value & 0x0F);
}

inline uint16_t bcd2bin16 (uint16_t value)
{
	return (uint16_t)bcd2bin8(value >> 8) * 100 + (uint16_t)bcd2bin8(value & 0x00FF);
}

inline uint32_t bcd2bin32 (uint32_t value)
{
	return (uint32_t)bcd2bin16(value >> 16) * 10000 + (uint32_t)bcd2bin16(value & 0x0000FFFF);
}

bool CDL645::OnProcess(ST_BYTE* pbuf, int len)
{
	if(pbuf[8] == 0x81)
	{
        uint32_t typ = (pbuf[11]-0x33)*256 + (pbuf[10]-0x33);
        switch(typ)
        {
            /*
            case 0xC032: {
                    uint64_t value = pbuf[17]-0x33;
                    value = value << 8;
                    value = value & (pbuf[16]-0x33);
                    value = value << 8;
                    value = value & (pbuf[15]-0x33);
                    value = value << 8;
                    value = value & (pbuf[14]-0x33);
                    value = value << 8;
                    value = value & (pbuf[13]-0x33);
                    value = value << 8;
                    value = value & (pbuf[12]-0x33);
                    this->UpdateValue(0,value);
                } break; */
            case 0x9010: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(0,float(fvalue));
                } break;
            case 0x9020: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(1,float(fvalue));
                } break;
            case 0x9110: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(2,float(fvalue));
                } break;
            case 0x9120: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(3,float(fvalue));
                } break;
            case 0x9130: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(4,float(fvalue));
                } break;
            case 0x9140: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(5,float(fvalue));
                } break;
            case 0x9150: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(6,float(fvalue));
                } break;
            case 0x9160: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(7,float(fvalue));
             } break;
            case 0xA010: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                           ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(8,float(fvalue));
                } break;
            /*case 0xB010: {
                    char str[64];
                    sprintf(str,"%d月%d日 %d:%d",pbuf[15]-0x33,pbuf[14]-0x33,pbuf[13]-0x33,pbuf[12]-0x33);
                    this->UpdateValue(9,str);
                } break;*/
            case 0xB611: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*1;
                    this->UpdateValue(9,float(fvalue));
                } break;
            case 0xB612: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*1;
                    this->UpdateValue(10,float(fvalue));
                } break;
            case 0xB613: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*1;
                    this->UpdateValue(11,float(fvalue));
                } break;
            case 0xB621: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.01;
                    this->UpdateValue(12,float(fvalue));
                } break;
            case 0xB622: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.01;
                    this->UpdateValue(13,float(fvalue));
                } break;
            case 0xB623: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.01;
                    this->UpdateValue(14,float(fvalue));
                } break;
            case 0xB630: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                           ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(15 , (((pbuf[14]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB631: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                           ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(16 , (((pbuf[14]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB632: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                           ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(17 , (((pbuf[14]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB633: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                           ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(18 , (((pbuf[14]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB640: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0x70)>>4)*1000)*0.01;
                    this->UpdateValue(19,(((pbuf[13]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB641: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0x70)>>4)*1000)*0.01;
                    this->UpdateValue(20,(((pbuf[13]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB642: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0x70)>>4)*1000)*0.01;
                    this->UpdateValue(21,(((pbuf[13]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB643: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0x70)>>4)*1000)*0.01;
                    this->UpdateValue(22,(((pbuf[13]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                } break;
            case 0xB650: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.001;
                    this->UpdateValue(23,float(fvalue));
                } break;
            case 0xB651: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.001;
                    this->UpdateValue(24,float(fvalue));
                } break;
            case 0xB652: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.001;
                    this->UpdateValue(25,float(fvalue));
                } break;
            case 0xB653: {
                    float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                           ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000)*0.001;
                    this->UpdateValue(26,float(fvalue));
                } break;
            case 0xC020: {
                    uint8_t value = pbuf[12]-0x33;
                    ST_BYTE _value = 0;
                    _value = (value & 0x01)? 1:0;
                    this->UpdateValue(50,_value);
                    _value = (value & 0x02)? 1:0;
                    this->UpdateValue(51,_value);
                    _value = (value & 0x04)? 1:0;
                    this->UpdateValue(52,_value);
                    _value = (value & 0x10)? 1:0;
                    this->UpdateValue(53,_value);
                    _value = (value & 0x20)? 1:0;
                    this->UpdateValue(54,_value);
                } break;
            case 0xC021: {
                    uint8_t value = pbuf[12]-0x33;
                    ST_BYTE _value = 0;
                    _value = (value & 0x01)? 1:0;
                    this->UpdateValue(100,_value);
                    _value = (value & 0x02)? 1:0;
                    this->UpdateValue(101,_value);
                    _value = (value & 0x04)? 1:0;
                    this->UpdateValue(102,_value);

                    _value = (value & 0x10)? 1:0;
                    this->UpdateValue(103,_value);
                    _value = (value & 0x20)? 1:0;
                    this->UpdateValue(104,_value);
                    _value = (value & 0x40)? 1:0;
                    this->UpdateValue(105,_value);
                } break;
            case 0xC022: {
                    uint8_t value = pbuf[12]-0x33;
                    ST_BYTE _value = 0;
                    _value = (value & 0x01)? 1:0;
                    this->UpdateValue(150,_value);
                    _value = (value & 0x02)? 1:0;
                    this->UpdateValue(151,_value);
                    _value = (value & 0x04)? 1:0;
                    this->UpdateValue(152,_value);
                    _value = (value & 0x08)? 1:0;
                    this->UpdateValue(153,_value);

                    _value = (value & 0x10)? 1:0;
                    this->UpdateValue(154,_value);
                    _value = (value & 0x20)? 1:0;
                    this->UpdateValue(155,_value);
                    _value = (value & 0x40)? 1:0;
                    this->UpdateValue(156,_value);
                } break;
            case 0x9011: {
                float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(27,float(fvalue));
            } break;
            case 0x9012: {
                float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(28,float(fvalue));
            } break;
            case 0x9013: {
                float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(29,float(fvalue));
            } break;
            case 0x9014: {
                float fvalue = (float)(((pbuf[12]-0x33)&0x0f) + (((pbuf[12]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[13]-0x33)&0x0f)*100 + (((pbuf[13]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[14]-0x33)&0x0f)*10000 + (((pbuf[14]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[15]-0x33)&0x0f)*1000000 + (((pbuf[15]-0x33)&0xf0)>>4)*10000000)*0.01;
                    this->UpdateValue(30,float(fvalue));
            } break;
            default:
                break;
        }
	}
	else if(pbuf[8] == 0xC1 && pbuf[9] == 0x01)
	{
	    switch(pbuf[10]-0x33)
	    {
	        case 0x01: this->ShowMessage("Device error response，the reason：invalid data."); break;
	        case 0x02: this->ShowMessage("Device error response，the reason：wrong datatype."); break;
	        case 0x04: this->ShowMessage("Device error response，the reason：wrong password."); break;
	        case 0x10: this->ShowMessage("Device error response，the reason：year time zone overflow."); break;
	        case 0x20: this->ShowMessage("Device error response，the reason：day tiem zone overflow."); break;
	        case 0x40: this->ShowMessage("Device error response，the reason：rate overflow."); break;
	        default: this->ShowMessage("Device error response，the reason：Undefined."); break;
	    }
	}
	return true;
}

void    CDL645::ReadData(ST_UINT16 wAddr)
{
//	ST_BYTE sendbuf[256];
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[ 0] = 0x68;
	sendbuf[ 1] = (info.Addressex[10]-'0') * 16 + (info.Addressex[11]-'0');
	sendbuf[ 2] = (info.Addressex[ 8]-'0') * 16 + (info.Addressex[ 9]-'0');
	sendbuf[ 3] = (info.Addressex[ 6]-'0') * 16 + (info.Addressex[ 7]-'0');
	sendbuf[ 4] = (info.Addressex[ 4]-'0') * 16 + (info.Addressex[ 5]-'0');
	sendbuf[ 5] = (info.Addressex[ 2]-'0') * 16 + (info.Addressex[ 3]-'0');
	sendbuf[ 6] = (info.Addressex[ 0]-'0') * 16 + (info.Addressex[ 1]-'0');
	sendbuf[ 7] = 0x68;
	sendbuf[ 8] = 0x01;
	sendbuf[ 9] = 0x02;
	sendbuf[10] =  (wAddr & 0x00ff)       + 0x33;
	sendbuf[11] = ((wAddr & 0xff00) >> 8) + 0x33;
    ST_BYTE bySum = 0x00;
	for(int i = 0; i < 12; ++i)
		bySum += sendbuf[i];

	sendbuf[12] = bySum;
	sendbuf[13] = 0x16;
	this->Send(sendbuf, 14);
}


