// DL645.h: interface for the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DL645_H__512F5502_59E3_4165_B10E_D2DF3896304B__INCLUDED_
#define _DL645_H__512F5502_59E3_4165_B10E_D2DF3896304B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif


#include "Protocol.h"
class CDL645 : public Protocol
{
public:
	CDL645();
	virtual ~CDL645();
    void	Init();
    void	Uninit();

    void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
	void        ReadData  (ST_UINT32 wAddr);
public:
	ST_INT m_readIndex;

    unsigned char m_addrarea[6];
};


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif


#endif // _DL645_H__512F5502_59E3_4165_B10E_D2DF3896304B__INCLUDED_
