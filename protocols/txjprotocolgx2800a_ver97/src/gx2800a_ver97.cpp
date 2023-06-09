
#include "gx2800a_ver97.h"

Protocol * CreateInstace()
{
	return new GX2800A();
}


inline ST_BYTE FromBCD_BYTE(ST_BYTE value)
{
	return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

ST_UINT16 FromBCD_WORD(ST_UINT16 value)
{
	return (ST_UINT16)FromBCD_BYTE((value & 0xFF00) >> 8) * 100 + (ST_UINT16)FromBCD_BYTE(value & 0x00FF);
}

ST_UINT32 FromBCD_DWORD(ST_UINT32 value)
{
	return (ST_UINT32)FromBCD_WORD((value & 0xFFFF0000) >> 16) * 10000 + (ST_UINT32)FromBCD_WORD(value & 0x0000FFFF);
}

inline ST_BYTE get_check_sum (ST_BYTE * data, int len)
{
	ST_BYTE sum = 0;

	while (len --> 0)
		sum += data[len];
	return sum;
}

GX2800A::GX2800A()
{}

GX2800A::~GX2800A()
{}

void GX2800A::Init ()
{
    m_curreadIndex = 0;
}

void GX2800A::Uninit()
{}

void GX2800A::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	if (!this->GetCurPort())
		return;
	if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
	{
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
	int len = this->GetCurPort()->PickBytes(pbuf, 4, 3000);
	if (len < 4) {
        this->ShowRecvFrame(pbuf, len);
		this->ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT star = 0;
	for (; star < len; ++star) {
		if(pbuf[star] == 0xAA)
			break;
	}
	if(star > 0) {
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	if(star == len) {
		//全是乱码
		this->ShowRecvFrame(pbuf, len);
		this->ShowMessage ("Garbled code, clear buffer.");
		this->GetCurPort()->Clear();
		return;
	}
	len = this->GetCurPort()->PickBytes(pbuf, 4, 2000);
	if (pbuf[star + 1] != (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
	{
	    this->ShowRecvFrame(pbuf, len);
		this->ShowMessage ("Address does not match.");
		this->GetCurPort()->Clear();
		return;
	}
	len = this->GetCurPort()->PickBytes(pbuf, 6, 2000);

	int nlen = pbuf[5] + 8;
	len = this->GetCurPort()->ReadBytes(pbuf, nlen, 2000);
	if (nlen != len) {
	    this->ShowRecvFrame(pbuf, len);
		this->ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
		return;
	}
	if (pbuf[len - 2] != get_check_sum(pbuf, nlen - 2) || pbuf[len - 1] != 0x0D)
	{
	    this->ShowRecvFrame(pbuf, len);
		this->ShowMessage ("Check error!");
		this->GetCurPort()->Clear();
		return;
	}
	readed = len;
}

void GX2800A::SendQuery(ST_UINT16 addr, ST_BYTE len)
{
	ST_BYTE data[16] = {0};
	data[0] = 0xAA;
	data[1] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[2] = 0xBB;

	memcpy(data + 3, &addr, sizeof(addr));

	data[5] = len;
	data[6] = get_check_sum(data, 6);
	data[7] = 0x0D;

	this->Send(data, 8);
}

ST_BOOLEAN GX2800A::OnSend()
{
	if (!this->GetDevice() || !this->GetDevice()->GetDeviceInfo()->DataAreas)
		return true;
	SendQuery (this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].addr,
			   this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].len);
	m_curreadIndex = (++m_curreadIndex % this->GetDevice()->GetDeviceInfo()->DataAreasCount);
	return true;
}

ST_BOOLEAN GX2800A::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	if(m_curreadIndex >= 0
		&& m_curreadIndex < this->GetDevice()->GetDeviceInfo()->DataAreasCount
		&& pbuf[2] == 0xBB)
		ProcessMemory(pbuf + 6, pbuf[5]);
	return true;
}

void GX2800A::ProcessMemory(ST_BYTE* buf, ST_BYTE count)
{
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].itemCount;
	for(ST_INT k = 0; k < itemsize; k++)
	{
		const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[k];
		if(count < (itemref.addr + itemref.dataLen))
			continue;

		switch(itemref.dataType) {
		case VALType_Char :
		case VALType_SByte:
			ProcessMemorySByte    (buf, itemref); break;
		case VALType_Int16:
			ProcessMemoryInt16    (buf, itemref); break;
		case VALType_Int32:
			ProcessMemoryInt32    (buf, itemref); break;
		case VALType_Byte :
			ProcessMemoryByte     (buf, itemref); break;
		case VALType_UInt16:
			ProcessMemoryUInt16   (buf, itemref); break;
		case VALType_UInt32:
			ProcessMemoryUInt32   (buf, itemref); break;
		case VALType_Float:
			ProcessMemorySingle   (buf, itemref); break;
		case VALType_Boolean:
			ProcessMemoryBoolean  (buf, itemref); break;
		case VALType_String:
			ProcessMemoryString   (buf, itemref); break;
		case VALType_Binary:
			ProcessMemoryBytes    (buf, itemref); break;
		case VALType_Double:
			ProcessMemoryDouble   (buf, itemref); break;
		case VALType_Decimal:
			ProcessMemoryDecimal  (buf, itemref); break;
		case VALType_DateTime:
			ProcessMemoryDateTime (buf, itemref); break;
		case VALType_Int64:
			ProcessMemoryInt64    (buf, itemref); break;
		case VALType_UInt64:
			ProcessMemoryUInt64   (buf, itemref); break;
		default: break;
		}
	}
}

inline ST_UINT16 bswap16 (ST_UINT16 value)
{
	return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}
inline ST_UINT32 bswap32 (ST_UINT32 value)
{
	return (((ST_UINT32)bswap16(value & 0x0000FFFF) << 16) | bswap16((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 bswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)bswap32(value & 0x00000000FFFFFFFF) << 32) | bswap32((value & 0xFFFFFFFF00000000) >> 32));
}
inline ST_UINT32 wswap32 (ST_UINT32 value)
{
	return (((value & 0x0000FFFF) << 16) | ((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 wswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)wswap32(value & 0x00000000FFFFFFFF) << 32) | wswap32((value & 0xFFFFFFFF00000000) >> 32));
}

inline ST_UINT32 htonl1(ST_UINT32 dv)
{
	ST_UINT16 dwHTemp = (ST_UINT16)((dv&0xffff0000)>>16);
	ST_UINT16 dwLTemp = (ST_UINT16)(dv&0x0000ffff);
	ST_UINT16 wLTemp  = bswap16(dwLTemp);
	ST_UINT16 wHTemp  = bswap16(dwHTemp);
	ST_UINT32 dwTemp = wHTemp;
	return ((dwTemp<<16)|wLTemp);
}

void  GX2800A::ProcessMemorySByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if(itemref.endBit - itemref.beginBit < 7)
		{
			value = (value & (0xFF << (itemref.beginBit -1))) & (0xFF >> (8 - itemref.endBit));
			value = value >> (itemref.beginBit - 1);
		}
		if(itemref.coeficient < 0)
		{
			value = !value;
		}
		this->UpdateValue(itemref.id, (ST_BYTE)value);
		//this->UpdateValue(k,(ST_BYTE)value);
	}
	else if(itemref.dataLen == 2)
	{
		ST_INT16 value = 0x0000;
		if((itemref.codeType == 1))
		{
			value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
		}
		else
		{
			// value = *(ST_INT16*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof (value));
		}
		if(itemref.endBit - itemref.beginBit < 15)
		{
			value = (value & (0xFFFF << (itemref.beginBit -1))) & (0xFFFF >> (16 - itemref.endBit));
			value = value >> (itemref.beginBit - 1);
		}
		if(itemref.coeficient < 0)
		{
			value = !value;
		}
		this->UpdateValue(itemref.id, (ST_BYTE)value);
		//this->UpdateValue(k,(ST_BYTE)value);
	}
}

void  GX2800A::ProcessMemoryInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_FLOAT)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT16* dvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(dvalue[1]*10000 + dvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT16* dvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(bswap16(dvalue[3])*1000000000000 + bswap16(dvalue[2])*100000000 + bswap16(dvalue[1])*10000 + bswap16(dvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemoryInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_DOUBLE)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
            ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
            memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemoryByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ProcessMemorySByte(buf, itemref);
}

void  GX2800A::ProcessMemoryUInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue  = (ST_FLOAT)(value);
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)value;
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemoryUInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_DOUBLE)(value);
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
	}
	else if(itemref.dataLen == 3)
	{
		if(itemref.codeType == 2)
		{
			ST_UINT32 value = 0;
			memcpy (&value, buf + itemref.addr, itemref.dataLen);
			fvalue = FromBCD_DWORD(value);
		}
		else if (itemref.codeType == 0)
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, itemref.dataLen);
			fvalue = value;
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)value;
		}
		else if((itemref.codeType == 6))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(pbcdvalue[1]*10000 + pbcdvalue[0]);
		}
	}
	else if(itemref.dataLen == 8)
	{
		if((itemref.codeType == 10))
		{
			ST_UINT16* pbcdvalue = (ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(bswap16(pbcdvalue[3])*1000000000000 + bswap16(pbcdvalue[2])*100000000 + bswap16(pbcdvalue[1])*10000 + bswap16(pbcdvalue[0]));
		}
	}
	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemorySingle(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_FLOAT fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue = (ST_FLOAT)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_FLOAT)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
	}

	if(itemref.coeficient != 0)
	{
		fvalue = fvalue * itemref.coeficient;
	}
	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemoryBoolean(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_BYTE value = (ST_BOOLEAN)(*(buf + itemref.addr));
	this->UpdateValue(itemref.id, value);
}

void  GX2800A::ProcessMemoryString(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  GX2800A::ProcessMemoryBytes(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  GX2800A::ProcessMemoryDouble(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_DOUBLE fvalue = 0;
	if(itemref.dataLen == 1)
	{
		ST_BYTE value = *(buf + itemref.addr);
		if((itemref.codeType == 2) || (itemref.codeType == 3))
		{
			fvalue = FromBCD_BYTE(value);
		}
		else fvalue  = (ST_DOUBLE)(*((ST_CHAR*)&value));
	}
	else if(itemref.dataLen == 2)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT16 value = FromBCD_WORD(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 3))
		{
			ST_UINT16 value = FromBCD_WORD(bswap16(*(ST_UINT16*)(buf + itemref.addr)));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT16 value = *(ST_UINT16*)(buf + itemref.addr);
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT16 value = bswap16(*(ST_UINT16*)(buf + itemref.addr));
			fvalue = (ST_DOUBLE)(*((ST_INT16*)&value));
		}
	}
	else if(itemref.dataLen == 4)
	{
		if((itemref.codeType == 2))
		{
			ST_UINT32 value = FromBCD_DWORD(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
        else if((itemref.codeType == 3))
		{
			ST_UINT32 value = FromBCD_DWORD(bswap32(*(ST_UINT32*)(buf + itemref.addr)));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 0))
		{
			ST_UINT32 value = 0;//*(ST_UINT32*)(buf + itemref.addr);
			memcpy (&value, buf + itemref.addr, sizeof(value));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 1))
		{
			ST_UINT32 value = bswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
		else if((itemref.codeType == 4))
		{
			ST_UINT32 value = htonl1(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));

		}
		else if((itemref.codeType == 5))
		{
			ST_UINT32 value = wswap32(*(ST_UINT32*)(buf + itemref.addr));
			fvalue = (ST_FLOAT&)(*((ST_INT32*)&value));
		}
	}
	else if (itemref.dataLen == 8)
	{
		switch (itemref.codeType) {
			case 0: {
				ST_UINT64 value = 0;
				memcpy (&value, buf + itemref.addr, sizeof(value));
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
			case 1: {
				ST_UINT64 value = bswap64 (*(ST_UINT64*)(buf + itemref.addr));
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
			case 4: {
			} break;
			case 5: {
				ST_UINT64 value = wswap64 (*(ST_UINT64*)(buf + itemref.addr));
				fvalue = (*((ST_DOUBLE*)&value));
			} break;
		}
	}

	if(itemref.coeficient != 0)
		fvalue = fvalue * itemref.coeficient;

	this->UpdateValue(itemref.id, fvalue);
}

void  GX2800A::ProcessMemoryDecimal(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  GX2800A::ProcessMemoryDateTime(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

static bool BytesToUInt64 (const ST_BYTE* buf, ST_INT codetype, ST_UINT64& value)
{
	switch (codetype) {
		case 0: {
			memcpy (&value, buf, sizeof(value));
		} return true;
		case 1: {
			value = bswap64 (*((ST_UINT64*)buf));
		} return true;
		case 4: {
		} return false;
		case 5: {
			value = wswap64 (*((ST_UINT64*)buf));
		} return true;
	}
	return false;
}

void  GX2800A::ProcessMemoryInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if (itemref.dataLen < 8) {
		ProcessMemoryInt32 (buf, itemref);
		return;
	}
	ST_INT64   value  = 0;
	ST_UINT64& valref = *((ST_UINT64*)&value);
	if (BytesToUInt64 (buf + itemref.addr, itemref.codeType, valref)) {
		if(itemref.coeficient != 0)
			value += itemref.coeficient;
		this->UpdateValue(itemref.id, (ST_DOUBLE)value);
	}
}

void  GX2800A::ProcessMemoryUInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	if (itemref.dataLen < 6) {
		ProcessMemoryUInt32 (buf, itemref);
		return;
	}
	ST_UINT64 value = 0;
	if (itemref.dataLen == 6) {
		switch (itemref.codeType) {
			case 0: {
			memcpy (&value, buf + itemref.addr, 6);
			if(itemref.coeficient != 0)
				value += itemref.coeficient;
			this->UpdateValue(itemref.id, (ST_DOUBLE)value);
			} return;
			case 1: {
			value = (ST_UINT64)(buf[itemref.addr    ]) * 0x10000000000
				  + (ST_UINT64)(buf[itemref.addr + 1]) * 0x100000000
				  + (ST_UINT64)(buf[itemref.addr + 2]) * 0x1000000
				  + (ST_UINT64)(buf[itemref.addr + 3]) * 0x10000
				  + (ST_UINT64)(buf[itemref.addr + 4]) * 0x100
				  + (ST_UINT64)(buf[itemref.addr + 5]);
			if(itemref.coeficient != 0)
				value += itemref.coeficient;
			this->UpdateValue(itemref.id, (ST_DOUBLE)value);
			} return;
		}
	}

	if (BytesToUInt64 (buf + itemref.addr, itemref.codeType, value)) {
		if(itemref.coeficient != 0)
			value += itemref.coeficient;
		this->UpdateValue(itemref.id, (ST_DOUBLE)value);
	}
}

ST_BOOLEAN GX2800A::IsSupportEngine(ST_INT engineType)
{
	return engineType == 1; // ENGINE_POLLING
}
