// JD200.h: interface for the CJD200 class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _JD200_H__7D9D9E78_9EA5_4959_A859_BDBD948ADE11__INCLUDED_
#define _JD200_H__7D9D9E78_9EA5_4959_A859_BDBD948ADE11__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif


#include "Protocol.h"
class CJD200 : public Protocol  
{
public:
	CJD200();
	virtual ~CJD200();
	
    void	Init();
    void	Uninit();
	
    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
	
public:
	void  SendReadCmd(ST_BYTE code,ST_UINT16 readAddr,ST_UINT16 count);
	void  ProcessMemory0x01(ST_BYTE* buf,ST_BYTE count);
	void  ProcessMemory0x02(ST_BYTE* buf,ST_BYTE count);
	void  ProcessMemory0x03(ST_BYTE* buf,ST_BYTE count);
	void  SendYK(ST_BYTE bycid,ST_UINT16 writeAddr,ST_BOOLEAN bIsOn);
	void  SendSingleWriteCmd(ST_UINT16 readAddr,ST_UINT16 wValue);
	void  ReadFix(void);
	void  FillWORD(ST_BYTE* buf,ST_UINT16 v);
	void  TimeSync();	
	void  Explainsoe(ST_BYTE* buf,int count);
public:
	ST_BOOLEAN m_bTask;
	ProtocolTask m_curTask;
	int m_curreadIndex;
	int m_readIndex;
	int timecount,newminute,oldminute;
	int readindex;
	ST_BYTE iscleansoeflag; //是史清SOE
	time_t   m_tmSend;
	int      m_nWaitTime;
	ST_BOOLEAN    m_bbtncontrol;
	int     m_nykpoint;
	ST_BOOLEAN    m_nison;
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


#endif // _JD200_H__7D9D9E78_9EA5_4959_A859_BDBD948ADE11__INCLUDED_
