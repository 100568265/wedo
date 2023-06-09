// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "DL645.h"
#include "Device.h"

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
}

CDL645::~CDL645()
{
}

void CDL645::Init()
{
    m_readIndex = 0;
	memset (m_addrarea, 0, sizeof(m_addrarea));
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
		len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
		if((pbuf[0] == 0x68) && (pbuf[7] == 0x68))
		{
			ST_INT ndatalen = pbuf[9] + 12;
			len =this->GetCurPort()->PickBytes(pbuf, ndatalen, 5000);
			if(len!=ndatalen)
                this->GetCurPort()->PickBytes(pbuf, ndatalen, 2000);
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
}

bool CDL645::OnSend()
{
	switch(m_readIndex) {
		case 0:  ReadData (0x0201FF00); // 电压
            break;
        case 1:  ReadData (0x0202FF00); // 电流
            break;
        case 2:  ReadData (0x0203FF00); // 瞬时有功功率
            break;
        case 3:  ReadData (0x0204FF00); // 瞬时无功功率
            break;
        case 4:  ReadData (0x0205FF00); // 瞬时视在功率
            break;
        case 5:  ReadData (0x0206FF00); // 功率因数
            break;

        case 6:  ReadData (0x00000000); // (当前)组合有功总电能
            break;

        case 7:  ReadData (0x00010000); // (当前)正向有功总电能
            break;
		case 8:  ReadData (0x00010100); // (当前)正向有功费率1电能
            break;
        case 9:  ReadData (0x00010200); // (当前)正向有功费率2电能
            break;
        case 10:  ReadData (0x00010300); // (当前)正向有功费率3电能
            break;
        case 11:  ReadData (0x00010400); // (当前)正向有功费率4电能
            break;


        case 12:  ReadData (0x00020000); // (当前)反向有功总电能
            break;
		case 13:  ReadData (0x00020100); // (当前)反向有功费率1电能
            break;
        case 14:  ReadData (0x00020200); // (当前)反向有功费率2电能
            break;
        case 15:  ReadData (0x00020300); // (当前)反向有功费率3电能
            break;
        case 16:  ReadData (0x00020400); // (当前)反向有功费率4电能
            break;
        case 17:  ReadData (0x00030000); // (当前)组合无功1总电能
            break;
        case 18:  ReadData (0x00010001); // (上1结算日)正向有功总电能
            break;
		case 19:  ReadData (0x00010101); // (上1结算日)正向有功费率1电能
            break;
        case 20:  ReadData (0x00010201); // (上1结算日)正向有功费率2电能
            break;
        case 21:  ReadData (0x00010301); // (上1结算日)正向有功费率3电能
            break;
        case 22:  ReadData (0x00010401); // (上1结算日)正向有功费率4电能
            break;
        case 23:  ReadData (0x01010000); // (当前)正向有功总最大需量
            break;
        case 24:  ReadData (0x01010001); // (上1结算日)正向有功总最大需量
            break;
        case 25:  ReadData (0x00830000); // 当前正向有功谐波总电能
            break;
/*        case 26:  ReadData (0x04000306); // 电流互感器变比
            break;*/
        case 27:  ReadData (0x000B0000); // 当前结算周期组合有功总累计用电量
            break;
        case 28:  ReadData (0x000B0001); // 上1结算周期组合有功总累计用电量
            break;

		default: ReadData (0x0201FF00);
			break;
	}
	//m_readIndex = ((++m_readIndex) % 17);
	m_readIndex = ((++m_readIndex) % 40);
	return true;
}

bool CDL645::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
	if(pbuf[8] == 0x91) {
        unsigned long datatype = 0;
        memcpy(&datatype, pbuf + 10, sizeof(datatype));
        switch (datatype) {
            case 0x0201FF00:
            case 0x35343233: { // 电压
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0xf0)>>4)*1000)*0.1;
                    this->UpdateValue(i, fvalue);
                }
            } break;

            case 0x0202FF00:
            case 0x35353233: { // 电流
                float fvalue = 0;
                for(int i = 0; i < 3; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.001;
                    fvalue = ((pbuf[16+i*3]- 0x33)& 0x80)? (fvalue * -1.0) : fvalue;
                    this->UpdateValue(i +  3, fvalue);
                }
            } break;

            case 0x0203FF00:
            case 0x35363233: { // 瞬时有功功率
                float fvalue = 0;
                for(int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(i +  6, (((pbuf[16+i*3]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                }
            } break;

            case 0x0204FF00:
            case 0x35373233: { // 瞬时无功功率
                float fvalue = 0;
                for(int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(i + 10, (((pbuf[16+i*3]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                }
            } break;

            case 0x0205FF00:
            case 0x35383233: { // 瞬时视在功率
                float fvalue = 0;
                for(int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;
                    this->UpdateValue(i + 14, (((pbuf[16+i*3]-0x33) & 0x80) ? -1.0: 1.0) * fvalue);
                }
            } break;

            case 0x0206FF00:
            case 0x35393233: { // 功率因数
                float fvalue = 0;
                for (int i = 0; i < 4; ++i)
                {
                    fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0xf0)>>4)*1000)*0.001;
                    this->UpdateValue(i + 18, fvalue);
                }
            } break;


            case 0x00000000: // (当前)组合有功总电能  其它的也在这里处理 ???
            case 0x33333333: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0x70) >> 4) * 10000000)*0.01;
                fvalue = ((pbuf[17] - 0x33) & 0x80) ? (fvalue * -1.0) : fvalue;
                this->UpdateValue(22, fvalue);
            } break;

            case 0x00010000: // (当前)正向有功总电能
            case 0x33343333: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(23, fvalue);
            } break;


            case 0x00010100: // (当前)正向有功费率1电能
            case 0x33343433: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(24, fvalue);
            } break;

            case 0x00010200: // (当前)正向有功费率2电能
            case 0x33343533: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(25, fvalue);
            } break;

            case 0x00010300: // (当前)正向有功费率3电能
            case 0x33343633: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(26, fvalue);
            } break;

            case 0x00010400: // (当前)正向有功费率4电能
            case 0x33343733: {
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(27, fvalue);
            } break;


            case 0x00020000: // (当前)反向有功总电能
            case 0x33353333:{
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(28, fvalue);
            } break;

            case 0x00020100: // (当前)反向有功费率1电能
            case 0x33353433:{
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(29, fvalue);
            } break;

            case 0x00020200: // (当前)反向有功费率2电能
            case 0x33353533:{
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(30, fvalue);
            } break;

            case 0x00020300: // (当前)反向有功费率3电能
            case 0x33353633:{
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(31, fvalue);
            } break;

            case 0x00020400: // (当前)反向有功费率4电能
            case 0x33353733:{
                float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                    ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                    ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                    ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
                this->UpdateValue(32, fvalue);
            } break;



            case 0x00030000: // (当前)组合无功1总电能
            case 0x33363333: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0x70)>>4)*10000000)*0.01;
                fvalue = ((pbuf[17] - 0x33) & 0x80) ? (fvalue * -1.0) : fvalue;
                this->UpdateValue(33, fvalue);
            } break;

            case 0x00010001: // (上1结算日)正向有功总电能
            case 0x33343334: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(34, fvalue);
            } break;

            case 0x00010101: // (上1结算日)正向有功费率1电能
            case 0x33343434: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(35, fvalue);
            } break;

            case 0x00010201: // (上1结算日)正向有功费率2电能
            case 0x33343534: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(36, fvalue);
            } break;

            case 0x00010301: // (上1结算日)正向有功费率3电能
            case 0x33343634: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(37, fvalue);
            } break;

            case 0x00010401: // (上1结算日)正向有功费率4电能
            case 0x33343734: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(38, fvalue);
            } break;



            case 0x01010000: // (当前)正向有功总最大需量
            case 0x34343333: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000)*0.0001;
                this->UpdateValue(39, fvalue);
            } break;

            case 0x01010001: // (上1结算日)正向有功总最大需量
            case 0x34343334: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000)*0.0001;
                this->UpdateValue(40, fvalue);
            } break;



            case 0x00830000: // 当前正向有功谐波总电能
            case 0x33B63333: {
                float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                               ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                               ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                               ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
                this->UpdateValue(41, fvalue);
            } break;
/*
            case 0x04000306: // 电流互感器变比
            case 0x37333639: {
			float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
				((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
					((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0x70)>>4)*100000);
			this->UpdateValue(42, fvalue);
            } break;*/

        case 0x000B0000: // 当前结算周期组合有功总累计用电量
		case 0x333E3333: {
			float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
				((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
				((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
				((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(42, fvalue);
						 } break;

		case 0x000B0001: // 上1结算周期组合有功总累计用电量
		case 0x333E3334: {
			float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
				((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
				((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
				((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
			this->UpdateValue(43, fvalue);
						 } break;


            default: {
                this->ShowMessage("UnKown data id.");
            } break;
        }

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
