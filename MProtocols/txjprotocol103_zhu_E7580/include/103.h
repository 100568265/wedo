
#if !defined(_103_H__44BC1C30_030A_4CCA_A307_7491466B1678__INCLUDED_)
#define _103_H__44BC1C30_030A_4CCA_A307_7491466B1678__INCLUDED_

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class C103 : public Protocol
{
public:
	C103();
	virtual ~C103();
	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
	clock_t		Newcurtime,oldcurtime,sendserchtime,CLtime;
	void	SendASDU21();
	void	SendASDU6();
	void    ASDU2a(ST_BYTE* Rebuf);
	void    ASDU1(ST_BYTE* Rebuf);
	void	ASDU10(ST_BYTE* Rebuf,int len);
	void    ASDU2(ST_BYTE* Rebuf);
	ProtocolTask m_curTask;
	ST_BYTE    newday,oldday,sendflag;

    ST_BOOLEAN m_bTask;
    void	YKSelect (ProtocolTask & task);
	void	YKExecut (ProtocolTask & task);

    void	SFSelect (ProtocolTask & task);
	void	SFExecut (ProtocolTask & task);

	void    ReadSetValue(ST_BYTE group);//读取定值
	void    WriteSetFValue(ST_BYTE addr,float value);//写定值浮点型

    void    recordLog(const char *head,ST_BYTE *pbuf,int len);
	vector<int> fixVlgroup;  //x需要读取的定值组
    void    InitFixGroup();
    int     seq;

private:
	void	SendSetframe(ST_BYTE code);
	ST_BYTE    SendState;
	ST_BYTE    FCB;
	ST_BYTE    BreakCallState;
	void    EXpainYc(ST_BYTE* Rebuf);
	void    EXpainYx(ST_BYTE* Rebuf);
	void    SendAllSearch();

	void    TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec);

    void    ASDU0x32(ST_BYTE* Rebuf);
    void    ASDU0x28(ST_BYTE * Rebuf);
    void    ASDU0x29(ST_BYTE * Rebuf);
    void    ASDU0x2B(ST_BYTE * Rebuf);
	time_t  nT,oT;



	int     getDataAreaItem(int fun,int inf,ST_DataAreaItem& itemref);
	int     getfixValueGroup(int varid);
	int     getfixValueNo(int varid);
	int     getfixValueDataType(int varid);
	bool    is_sf;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API C103* CreateInstace();
#else
	C103* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(_103_H__44BC1C30_030A_4CCA_A307_7491466B1678__INCLUDED_)
