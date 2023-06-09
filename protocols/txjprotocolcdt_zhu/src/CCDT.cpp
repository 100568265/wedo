#include "CCDT.h"

const ST_BYTE g_check[256] =
{
	0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
	0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
	0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
	0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
	0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
	0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
	0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
	0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
	0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
	0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
	0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
	0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
	0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
	0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
	0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
	0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

CCDT::CCDT()
{
    //ctor
}

CCDT::~CCDT()
{
    //dtor
}

CCDT* CreateInstace()
{
    return new CCDT();
}

void CCDT::Init()
{
}

void CCDT::Uninit()
{

}

void CCDT::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	if(this->GetCurPort())
	{
		ST_INT  len = this->GetCurPort()->PickBytes(pbuf, 12, 1000);
		if(len >= 12)
		{
			ST_INT star = 0;
			for(star = 0; star < len; ++star)
			{
				if(pbuf[star] == 0xEB)
					break;
			}
	//		if(len==star)
			{
//				this->ShowReceiveFrame(pbuf,len);
		//		this->GetCurPort()->Clear();
			//	return;
			}
			if(star > 0)
			{
//				this->ShowReceiveFrame(pbuf,star);
				this->GetCurPort()->ReadBytes(pbuf, star);
				return;
			}
			len = this->GetCurPort()->PickBytes(pbuf, 12, 1000);
			if((pbuf[0] == 0xeb) && (pbuf[1] == 0x90) && (pbuf[2] == 0xeb) && (pbuf[3] == 0x90))
			{
				if (! VerifyCRC(&pbuf[6])) {
					ShowMessage ("Check error!");
					this->GetCurPort()->Clear();
					return;
				}
				ST_INT ndatalen = pbuf[8] * 6 + 12;
				
                len = this->GetCurPort()->PickBytes(pbuf, ndatalen, 3000);
				if(len == ndatalen)
				{
					len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
					if(ndatalen == len)
					{
						readed = len;
						return;
					}
				}
			}
			else
			{
//				this->ShowReceiveFrame(pbuf,len);
				this->GetCurPort()->Clear();
				return;
			}
		}
	}
}

ST_BOOLEAN	CCDT::OnSend()
{
	if (!curEngineType) // == ENGINE_FULLING
		sleep (1);      // 全双工通道时睡眠以空出CPU资源

    return true;
}

ST_BOOLEAN	CCDT::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
	if((pbuf[6] == 0x71) && (pbuf[7] == 0x61))
	{
		for(ST_INT i = 2; i < len / 6; ++i)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
				if(pbuf[i*6] >= 0xf0)
				{
					ST_INT nIndexYX = (pbuf[i*6] - 0xf0) * 32;
					for(ST_INT j = 0; j < 4; ++j)
					{
						for(ST_INT k = 0; k < 8; ++k)
						{
							ST_BYTE byValue = ((0x01 << k) & pbuf[i*6+j+1]) ? 0x01 : 0x00;
							this->UpdateValue(nIndexYX++, (ST_BYTE)byValue);
						}
					}
				}
				else
				{
					ST_INT nIndexYC = 0 + (pbuf[i*6])*2 + 10000;
					ST_FLOAT fValue = (ST_FLOAT)(pbuf[i*6+1] + (pbuf[i*6+2] & 0x07) * 256);
					if(pbuf[i*6+2] & 0x08) fValue = -(0x800 - fValue);
					this->UpdateValue(nIndexYC++, (ST_FLOAT)fValue);
					fValue = (ST_FLOAT)(pbuf[i*6+3] + (pbuf[i*6+4] & 0x07) * 256);
					if(pbuf[i*6+4] & 0x08) fValue = -(0x800 - fValue);
					this->UpdateValue(nIndexYC, (ST_FLOAT)fValue);
				}
			}
		}
	}

	if((pbuf[6] == 0x71) && (pbuf[7] == 0xC2))
	{
		for(int i = 2; i < len / 6; ++i)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
				if(pbuf[i*6]>=0xf0)
				{
					ST_INT nIndexYX = (pbuf[i*6]-0xf0)*32;
					for(ST_INT j=0;j<4;j++)
					{
						for(ST_INT k=0;k<8;k++)
						{
							ST_BYTE byValue = ((0x01<<k)&pbuf[i*6+j+1])?0x01:0x00;
							this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
						}
					}
				}
				else
				{
					ST_INT nIndexYC = 0+(pbuf[i*6])*2 + 14000;
					ST_FLOAT fValue = (ST_FLOAT)(pbuf[i*6+1] + (pbuf[i*6+2]&0x07)*256);
					if(pbuf[i*6+2]&0x08) fValue = -(0x800-fValue);
					this->UpdateValue(nIndexYC++,(ST_FLOAT)fValue);
					fValue = (ST_FLOAT)(pbuf[i*6+3] + (pbuf[i*6+4]&0x07)*256);
					if(pbuf[i*6+4]&0x08) fValue = -(0x800-fValue);
					this->UpdateValue(nIndexYC,(ST_FLOAT)fValue);
				}
			}
		}
	}
	
	if((pbuf[6] == 0x71) && (pbuf[7] == 0xb3))
	{
		for(ST_INT i=2;i<len/6;i++)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
				if(pbuf[i*6]>=0xf0)
				{
					ST_INT nIndexYX = (pbuf[i*6]-0xf0)*32;
					for(ST_INT j=0;j<4;j++)
					{
						for(ST_INT k=0;k<8;k++)
						{
							ST_BYTE byValue = ((0x01<<k)&pbuf[i*6+j+1])?0x01:0x00;
							this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
						}
					}
				}
				else
				{
					ST_INT nIndexYC = 0+(pbuf[i*6])*2 + 17000;
					ST_FLOAT fValue = (ST_FLOAT)(pbuf[i*6+1] + (pbuf[i*6+2]&0x07)*256);
					if(pbuf[i*6+2]&0x08) fValue = -(0x800-fValue);
					this->UpdateValue(nIndexYC++,(ST_FLOAT)fValue);
					fValue = (ST_FLOAT)(pbuf[i*6+3] + (pbuf[i*6+4]&0x07)*256);
					if(pbuf[i*6+4]&0x08) fValue = -(0x800-fValue);
					this->UpdateValue(nIndexYC,(ST_FLOAT)fValue);
				}
			}
		}
	}
	else if((pbuf[6] == 0x71) && (pbuf[7] == 0xf4))
	{
		ST_INT lastF = 0x00;
		for(ST_INT i = 2; i < len / 6; i++)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
			//	if(pbuf[i*6] == lastF)
			//		pbuf[i*6] = pbuf[i*6]+1;
				ST_INT nIndexYX = (pbuf[i*6] - 0xf0) * 32;
				for(ST_INT j = 0; j < 4; j++)
				{
					for(ST_INT k = 0; k < 8; k++)
					{
						ST_BYTE byValue = ((0x01 << k) & pbuf[i*6+j+1]) ? 0x01 : 0x00;
						this->UpdateValue(nIndexYX++, (ST_BYTE)byValue);
					}
				}
			//	lastF = pbuf[i*6];
			}
		}
	}
	else if((pbuf[6] == 0x71) && (pbuf[7] == 0x26))
	{
		ST_INT nCount = 0;
		ST_INT nMillSecond,nMinute;
		ST_INT nYXNo = 0;
		for(ST_INT i=2;i<len/6;i++)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
			    if(pbuf[i*6]>=0xf0)
				{
					ST_INT nIndexYX = (pbuf[i*6]-0xf0)*32;
					for(ST_INT j=0;j<4;j++)
					{
						for(ST_INT k=0;k<8;k++)
						{
							ST_BYTE byValue = ((0x01<<k)&pbuf[i*6+j+1])?0x01:0x00;
							this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
						}
					}
				}
				else if(pbuf[i*6] == 0x80)
				{
					if(nCount == 0)
					{
						nMillSecond = pbuf[i*6+1]+pbuf[i*6+2]*256+pbuf[i*6+3];
						nMinute     = pbuf[i*6+4];
						nCount++;
					}
				}
				else if((pbuf[i*6] == 0x81) && nCount)
				{
/*					CTime st = CTime::GetCurrentTime();
					nYXNo = pbuf[i*6+3]+(pbuf[i*6+4]&0x0f)*256;
					char name[64];
					char chSoeNAME[64];
					memcpy(chSoeNAME,this->GetCurLine()->m_lineParam.lineName,64);
					sprintf(name,getVarname(chSoeNAME,nYXNo));
					CString soestr;
					soestr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d ms %s 事件 %s ",
									st.GetYear(),st.GetMonth(),pbuf[i*6+2],pbuf[i*6+1],nMinute,nMillSecond/1000,nMillSecond%1000,
									name,pbuf[i*6+4]&0x80?"消失":"产生");
					this->ReportEvent(soestr,"soebj");
*/
					return true;
				}
			}
		}
	}
	else if((pbuf[6] == 0x71) && (pbuf[7] == 0x85))
	{
		for(int i=2;i<len/6;i++)
		{
			if(VerifyCRC(&pbuf[i*6]))
			{
				if(pbuf[i*6]>=0xf0)
				{
					int nIndexYX = (pbuf[i*6]-0xf0)*32;
					for(int j=0;j<4;j++)
					{
						for(int k=0;k<8;k++)
						{
							ST_BYTE byValue = ((0x01<<k)&pbuf[i*6+j+1])?0x01:0x00;
							this->UpdateValue(nIndexYX++,(ST_BYTE)byValue);
						}
					}
				}
				else
				{
					ST_INT nIndexYC = 0 + pbuf[i*6] - 0xa0 + 20000;
					ST_DOUBLE fValue = pbuf[i*6+1]+pbuf[i*6+2]*256+pbuf[i*6+3]*256*256;
					this->UpdateValue(nIndexYC,(ST_DOUBLE)fValue);
				}
			}
		}
	}
	return true;
}

ST_BOOLEAN	CCDT::IsSupportEngine(ST_INT engineType)
{
	curEngineType = engineType;
    return true;
}

ST_BOOLEAN CCDT::VerifyCRC (ST_BYTE *p)
{
	ST_BYTE cha, chb;
	cha = 0;
	for(int i = 0; i < 5; ++i)
	{
		chb = cha ^ *p++;
		cha = g_check[chb];
	}
	cha ^= 0xff;
	return (*p++ == cha);
}
