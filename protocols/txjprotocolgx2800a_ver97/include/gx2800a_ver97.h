
#ifndef _GX2800A_VER97_H_
#define _GX2800A_VER97_H_


#include "Protocol.h"     //规约父类
#include "Device.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class GX2800A : public Protocol
{
public:
	GX2800A();
	virtual ~GX2800A();

	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
private:

	void  ProcessMemory(ST_BYTE* buf,ST_BYTE count);

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


	void SendQuery(ST_UINT16 addr, ST_BYTE len);

	int m_curreadIndex;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif


#endif // _GX2800A_VER97_H_
