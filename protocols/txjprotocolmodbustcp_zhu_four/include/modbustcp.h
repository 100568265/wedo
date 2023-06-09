
#ifndef __MODBUSTCP_H__
#define __MODBUSTCP_H__

#include "Protocol.h"

typedef struct DataAreaItem ST_DataAreaItem;

class ModbusTcp : public Protocol
{
public:
	ModbusTcp();
	~ModbusTcp();

	void	Init();
	void	Uninit();

	void	OnRead(ST_BYTE * pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    void  SendReadCmd(ST_BYTE code, ST_UINT readAddr, ST_UINT count);
    void  SendWriteCmd(ST_UCHAR * pData, ST_UINT dataLen, ST_UINT addr);
    void  SendSingleWriteCmd(ST_FLOAT data, ST_INT addr, ST_INT nType);
    void  SendYK(ST_UINT writeAddr, ST_BOOLEAN bIsOn);
    void  SendPreYK(ST_UINT writeAddr, ST_BOOLEAN bIsOn);
    void  SendYT(ST_UINT writeAddr, ST_UINT wIndex, ST_BOOLEAN bIsOn);

	void  ProcessMemory(ST_BYTE* buf, ST_BYTE count);

	void  ProcessMemorySByte   (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryInt16   (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryInt32   (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryByte    (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryUInt16  (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryUInt32  (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemorySingle  (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryBoolean (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryString  (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryBytes   (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryDouble  (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryDecimal (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryDateTime(ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryInt64   (ST_BYTE*, const ST_DataAreaItem&);
	void  ProcessMemoryUInt64  (ST_BYTE*, const ST_DataAreaItem&);

private:
    void FillWORD(ST_BYTE* buf,ST_UINT v);

    ST_BOOLEAN m_bTask;
    ProtocolTask m_curTask;
    ST_INT m_curreadIndex;
    ST_INT m_readIndex;
};

_PROTOCOL_INTERFACE_DECLARATION_(ModbusTcp);

#endif // __MODBUSTCP_H__
