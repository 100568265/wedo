
#include "modbustcp.h"

#include "Device.h"
#include "EngineBase.h"
#include "Channel.h"

#pragma pack(push,1)

struct mbap_head
{
	inline ST_UINT16 get_t_id ()
		{ return (t_h_id / 256 + (t_h_id % 256) * 256); }
	inline ST_UINT16 get_p_id ()
		{ return (ptl_id / 256 + (ptl_id % 256) * 256); }
	inline ST_UINT16 get_len ()
		{ return (length / 256 + (length % 256) * 256); }

	inline void set_len (ST_UINT16 len)
		{ length = (len / 256 + (len % 256) * 256); }

	ST_UINT16  t_h_id;
	ST_UINT16  ptl_id;
	ST_UINT16  length;
	ST_BYTE    unit_id;
};

#pragma pack(pop)


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

inline ST_BYTE TOBCD_BYTE(ST_BYTE value)
{
	return ((value / 10) << 4) + (value % 10);
}

ST_UINT16 TOBCD_WORD(ST_UINT16 value)
{
	return ((ST_UINT16)TOBCD_BYTE((value & 0xFF00) >> 8) << 8) + (ST_UINT16)TOBCD_BYTE(value & 0x00FF);
}

ST_UINT32 TOBCD_DWORD(ST_UINT32 value)
{
	return ((ST_UINT32)TOBCD_WORD((value & 0xFFFF0000) >> 16) << 16) + (ST_UINT32)TOBCD_WORD(value & 0x0000FFFF);
}


_PROTOCOL_INTERFACE_IMPLEMENT_(ModbusTcp)

ModbusTcp::ModbusTcp()
{}

ModbusTcp::~ModbusTcp()
{}

void ModbusTcp::Init()
{}

void ModbusTcp::Uninit()
{}

void	ModbusTcp::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
	readed = 0;
	if(! this->GetCurPort())
		return;
	if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
	{
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 7, 2000);
	if(len < 7) {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}

	mbap_head * mh_ptr = (mbap_head*) pbuf;

	if(mh_ptr->get_p_id() != 0)
	{
		this->ShowMessage("Not a modbus protocol identifier!");
		this->ShowRecvFrame(pbuf, len);
		this->GetCurPort()->Clear();
		return;
	}

	if(mh_ptr->unit_id != (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
	{
		this->ShowMessage("Addresses do not match!");
		this->ShowRecvFrame(pbuf, len);
		this->GetCurPort()->Clear();
		return;
	}

	int datalen = mh_ptr->get_len() + 6;
	len = this->GetCurPort()->PickBytes(pbuf, datalen, 3000);
	if(datalen == len) {
		readed = this->GetCurPort()->ReadBytes(pbuf, datalen, 1000);
		return;
	}
	else
	{
		this->ShowMessage("Insufficient data length!");
		this->ShowRecvFrame(pbuf, len);
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN	ModbusTcp::OnSend()
{
    if (this->GetCurPort())
        this->GetCurPort()->Clear();

	m_bTask = false;
//	ShowMessage("Inter Send()");
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
	    ShowMessage("Inter HasTask");
		if(!strcmp(m_curTask.taskCmd,"singleread"))
		{
			SendReadCmd((ST_BYTE)m_curTask.taskCmdCode,m_curTask.taskAddr,m_curTask.taskAddr1);
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)
				SendPreYK(m_curTask.taskAddr,m_curTask.taskValue);
			else if(m_curTask.taskCmdCode == 1)
				SendYK(m_curTask.taskAddr,m_curTask.taskValue);
			else if(m_curTask.taskCmdCode == 2)
				SendReadCmd((ST_BYTE)m_curTask.taskCmdCode,m_curTask.taskAddr,m_curTask.taskAddr1);
			else
			{
                m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                memset(&m_curTask,0,sizeof(m_curTask));
				return false;
			}
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"singlewrite"))
		{
			// if(m_curTask.taskCmdCode == 2)
			// {
			// 	ST_UINT16 wIndex = m_curTask.taskParam[1];
			// 	wIndex = (wIndex<<8)| m_curTask.taskParam[0];
			// 	SendYT(m_curTask.taskAddr, wIndex, m_curTask.taskValue ? 1 : 0);
			// }
			// else
				SendSingleWriteCmd((ST_FLOAT)m_curTask.taskValue,m_curTask.taskAddr,m_curTask.taskAddr1);
			m_bTask = true;
		}
		else if(!strcmp(m_curTask.taskCmd,"multiwrite"))
		{
			//写定值
			SendWriteCmd(m_curTask.taskParam,m_curTask.taskParamLen,m_curTask.taskAddr);
			m_bTask = true;
		}
		return true;
	}
	const DeviceInfo* info = this->GetDevice()->GetDeviceInfo();
	if(info && info->DataAreasCount > 0)
	{
		if (m_readIndex >= info->DataAreasCount)
			m_readIndex = 0;
		m_curreadIndex = m_readIndex++;
		SendReadCmd(info->DataAreas[m_curreadIndex].readCode, (ST_UINT16)info->DataAreas[m_curreadIndex].addr,
			        info->DataAreas[m_curreadIndex].dataUnitLen > 1 ?
			        info->DataAreas[m_curreadIndex].len / info->DataAreas[m_curreadIndex].dataUnitLen : info->DataAreas[m_curreadIndex].len);
	}
	return true;
}

ST_BOOLEAN	ModbusTcp::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
	if(m_bTask)
	{
		if(!strcmp(m_curTask.taskCmd,"singlewrite") || !strcmp(m_curTask.taskCmd,"multiwrite"))
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else if(!strcmp(m_curTask.taskCmd,"singleread"))
		{
			m_curTask.taskResult.resultCode = 0;
			m_curTask.taskResult.resultDataLen = pbuf[8];
			memcpy(m_curTask.taskResult.resultData,&pbuf[9],m_curTask.taskResult.resultDataLen);
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
		else
		{
			m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
	}
	else if(m_curreadIndex >= 0 && m_curreadIndex < this->GetDevice()->GetDeviceInfo()->DataAreasCount
			&& pbuf[7] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
	{
		ProcessMemory(&pbuf[9], pbuf[8]);
	}
	return true;
}

ST_BOOLEAN	ModbusTcp::IsSupportEngine(ST_INT engineType)
{
    return engineType == EngineBase::ENGINE_POLLING;
}

void ModbusTcp::ProcessMemory(ST_BYTE* buf, ST_BYTE count)
{
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].itemCount;
	for(ST_INT k = 0; k < itemsize; k++)
	{
		const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].items[k];
		if(count < (itemref.addr + itemref.dataLen))
			continue;

		switch(itemref.dataType) {
		case VALType_Char :
		case VALType_SByte:     ProcessMemorySByte    (buf, itemref); break;
		case VALType_Int16:     ProcessMemoryInt16    (buf, itemref); break;
		case VALType_Int32:     ProcessMemoryInt32    (buf, itemref); break;
		case VALType_Byte :     ProcessMemoryByte     (buf, itemref); break;
		case VALType_UInt16:    ProcessMemoryUInt16   (buf, itemref); break;
		case VALType_UInt32:    ProcessMemoryUInt32   (buf, itemref); break;
		case VALType_Float:     ProcessMemorySingle   (buf, itemref); break;
		case VALType_Boolean:   ProcessMemoryBoolean  (buf, itemref); break;
		case VALType_String:    ProcessMemoryString   (buf, itemref); break;
		case VALType_Binary:    ProcessMemoryBytes    (buf, itemref); break;
		case VALType_Double:    ProcessMemoryDouble   (buf, itemref); break;
		case VALType_Decimal:   ProcessMemoryDecimal  (buf, itemref); break;
		case VALType_DateTime:  ProcessMemoryDateTime (buf, itemref); break;
		case VALType_Int64:     ProcessMemoryInt64    (buf, itemref); break;
		case VALType_UInt64:    ProcessMemoryUInt64   (buf, itemref); break;
		default: break;
		}
	}
}

// 01 02 -> 02 01
inline ST_UINT16 bswap16 (ST_UINT16 value)
{
	return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}
// 0x01 0x02 0x03 0x04 -> 0x04 0x03 0x02 0x01
inline ST_UINT32 bswap32 (ST_UINT32 value)
{
	return (((ST_UINT32)bswap16(value & 0x0000FFFF) << 16) | bswap16((value & 0xFFFF0000) >> 16));
}
// 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 -> 0x08 0x07 0x06 0x05 0x04 0x03 0x02 0x01
inline ST_UINT64 bswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)bswap32(value & 0x00000000FFFFFFFF) << 32) | bswap32((value & 0xFFFFFFFF00000000) >> 32));
}
// 0x01 0x02 0x03 0x04 -> 0x03 0x04 0x01 0x02
inline ST_UINT32 wswap32 (ST_UINT32 value)
{
	return (((value & 0x0000FFFF) << 16) | ((value & 0xFFFF0000) >> 16));
}
// 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 -> 0x07 0x08 0x05 0x06 0x03 0x04 0x01 0x02
inline ST_UINT64 wswap64 (ST_UINT64 value)
{
	return (((ST_UINT64)wswap32(value & 0x00000000FFFFFFFF) << 32) | wswap32((value & 0xFFFFFFFF00000000) >> 32));
}
// 0x01 0x02 0x03 0x04  -> 0x02 0x01 0x04 0x03
inline ST_UINT32 bswap16x2 (ST_UINT32 value)
{
	return (((ST_UINT32)bswap16(value >> 16) << 16) | bswap16(value & 0x0000FFFF));
}
// 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08  -> 0x02 0x01 0x04 0x03 0x06 0x05 0x08 0x07
inline ST_UINT64 bswap16x4 (ST_UINT64 value)
{
	return (((ST_UINT64)bswap16x2(value >> 32) << 32) | bswap16x2(value & 0x00000000FFFFFFFF));
}

void  ModbusTcp::ProcessMemorySByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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

void  ModbusTcp::ProcessMemoryInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemoryInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemoryByte(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ProcessMemorySByte(buf, itemref);
}

void  ModbusTcp::ProcessMemoryUInt16(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemoryUInt32(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemorySingle(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemoryBoolean(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{
	ST_BYTE value = (ST_BOOLEAN)(*(buf + itemref.addr));
	this->UpdateValue(itemref.id, value);
}

void  ModbusTcp::ProcessMemoryString(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  ModbusTcp::ProcessMemoryBytes(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  ModbusTcp::ProcessMemoryDouble(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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
			ST_UINT32 value = bswap16x2(*(ST_UINT32*)(buf + itemref.addr));
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
	//this->UpdateValue(k,fvalue);
}

void  ModbusTcp::ProcessMemoryDecimal(ST_BYTE* buf, const ST_DataAreaItem& itemref)
{

}

void  ModbusTcp::ProcessMemoryDateTime(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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

void  ModbusTcp::ProcessMemoryInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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

void  ModbusTcp::ProcessMemoryUInt64(ST_BYTE* buf, const ST_DataAreaItem& itemref)
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

void ModbusTcp::SendReadCmd(ST_BYTE code, ST_UINT readAddr, ST_UINT count)
{
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x00;
	sendbuf[3] = 0x00;
	sendbuf[4] = 0x00;
	sendbuf[5] = 0x06;
	sendbuf[6] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[7] = code;
	FillWORD(sendbuf +  8, readAddr);
	FillWORD(sendbuf + 10, count);
	this->Send(sendbuf, 12);
}

void ModbusTcp::SendWriteCmd(ST_UCHAR * pData, ST_UINT dataLen, ST_UINT addr)
{
	ST_BYTE sendbuf[1024] = {0};
	if (dataLen > 1000 || dataLen <= 0) return;
	int len = 0;
	int ndataLen = (dataLen % 2) ? (dataLen / 2 + 1) : (dataLen / 2);
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x00;
	sendbuf[3] = 0x00;
	sendbuf[4] = 0x00;
	sendbuf[5] = 0x09;
	sendbuf[6] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[7] = 0x10;
	FillWORD(sendbuf +  8, addr);
	FillWORD(sendbuf + 10, ndataLen);
	sendbuf[12] = ndataLen * 2;
	memcpy(sendbuf + 13, pData, dataLen);
	len = 13 + ndataLen * 2;

	this->Send(sendbuf, len);
}

void ModbusTcp::SendSingleWriteCmd(ST_FLOAT data, ST_INT addr, ST_INT nType)
{
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x00;
	sendbuf[3] = 0x00;
	sendbuf[4] = 0x00;

    sendbuf[5] = 0x06;
    sendbuf[6] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[7] = 0x06;
    FillWORD(sendbuf + 8, addr);

    sendbuf[10] = (ST_BYTE)data;
    sendbuf[11] = 0x00;

    this->Send(sendbuf, 12);
}

void ModbusTcp::SendYK(ST_UINT writeAddr, ST_BOOLEAN bIsOn)
{
//    ShowMessage("Enter YK");
	ST_BYTE sendbuf[16] = {0};
	int len = 0;
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x00;
	sendbuf[3] = 0x00;
	sendbuf[4] = 0x00;
	sendbuf[5] = 0x06;//长度
	sendbuf[6] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[7] = 0x05;
	FillWORD(sendbuf + 8, writeAddr);

	sendbuf[10] = (bIsOn ? 0xFF: 0x00);
	sendbuf[11] = 0x00;

	this->Send(sendbuf, 12);

}

void ModbusTcp::SendPreYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
//    ShowMessage("Enter PREYK");
	ST_BYTE sendbuf[16] = {0};
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x00;
	sendbuf[2] = 0x00;
	sendbuf[3] = 0x00;
	sendbuf[4] = 0x00;
	sendbuf[5] = 0x05;
	sendbuf[6] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[7] = 0x05;
	sendbuf[8] = 0x1f;
	if(bIsOn)
		sendbuf[9] = 0x11;
	else
		sendbuf[9] = 0x10;
	sendbuf[10] = 0xFF;
	sendbuf[11] = 0x00;

	this->Send(sendbuf, 12);
}

void ModbusTcp::SendYT(ST_UINT writeAddr,ST_UINT wIndex,ST_BOOLEAN bIsOn)
{
}

void ModbusTcp::FillWORD(ST_BYTE* buf,ST_UINT v)
{
	ST_BYTE* pv = (ST_BYTE*)&v;
	buf[0] = pv[1];
	buf[1] = pv[0];
}

