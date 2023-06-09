#ifndef PORTCOM_H
#define PORTCOM_H

#include "PortBase.h"


#ifdef _WIN32
   #define PORTCOM "COM"
#else
	#include <termios.h>
   #define PORTCOM "/dev/ttyS"
#endif

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

class PortCom:public PortBase
{
public:
    PortCom(Channel *channel,PortInfo *portInfo);
    virtual ~PortCom();
    ST_VOID					Init();
    ST_VOID					Uninit();
    ST_VOID					Open();
    ST_VOID					Close();
    ST_BOOLEAN 				IsOpened();
    ST_BOOLEAN 				Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size);
    ST_VOID					Recv();
protected:
	ST_INT					Set_Com(ST_INT com);
#ifdef _WIN32
	DCB						GetDCB();
	COMMTIMEOUTS			GetCOMMTIMEOUTS();
	ST_VOID					ReadToBuffer(COMSTAT ComStat);
#else
    ST_INT 					Set_Speed(ST_INT fd, ST_INT speed);
    ST_INT 					Set_Parity(ST_INT fd, ST_INT databits,ST_INT stopbits,ST_INT parity);
#endif
protected:
#ifdef _WIN32
	HANDLE					com_fd;
	OVERLAPPED				m_ovWrite;		//用于发送的数据OVERLAPPED结构
	OVERLAPPED				m_ovRead;		//用于接收数据的OVERLAPPED结构
	OVERLAPPED          	m_eov;
	DWORD					dwErrWord;
	COMSTAT 				comstat;
	DWORD					dwEventMask;
#else
    ST_INT 					com_fd;
	struct 					termios comOpt;
#endif
	ST_CHAR					comName[65];

};

#endif // PORTCOM_H
