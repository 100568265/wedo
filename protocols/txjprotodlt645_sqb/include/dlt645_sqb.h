// DL645.h: interface for the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DL645_H__6360728D_825D_4B7C_BF29_4095AB018740__INCLUDED_
#define _DL645_H__6360728D_825D_4B7C_BF29_4095AB018740__INCLUDED_

#if _MSC_VER > 1000
	#pragma once
#endif // _MSC_VER > 1000
#include "Protocol.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class CDL645 : public Protocol  
{
public:
	CDL645();
	virtual ~CDL645();
	void	Init();
	void	Uninit();

	void	OnRead(ST_BYTE* pbuf,ST_INT& readed);				
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
	ST_BOOLEAN  IsSupportEngine (ST_INT IsSupportEngine);

	void    ReadData(ST_UINT16 wAddr);

	ST_BYTE ReadIndex;
	ST_BYTE sendbuf[256];
public:
	ST_INT     m_readIndex;
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


#endif // _DL645_H__6360728D_825D_4B7C_BF29_4095AB018740__INCLUDED_
