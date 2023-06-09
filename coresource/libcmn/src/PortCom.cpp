//#include "stdafx.h"
#include "PortCom.h"
#include "Channel.h"

#include <assert.h>

PortCom::PortCom(Channel *channel,PortInfo *portInfo)
    :PortBase(channel,portInfo)
{
}

PortCom::~PortCom()
{
    Stop();
    Uninit();
}

ST_VOID PortCom::Init()
{
    if(m_Inited)
        return;
    assert(m_pPortInfo!=NULL);
#ifdef _WIN32
    memset( &m_ovRead,	0, sizeof( OVERLAPPED ) ) ;
    memset( &m_ovWrite, 0, sizeof( OVERLAPPED ) ) ;
    m_ovRead.hEvent =  CreateEvent( NULL,TRUE,FALSE,NULL ) ;
    m_ovWrite.hEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    com_fd=INVALID_HANDLE_VALUE;
#endif
    if(Set_Com(m_pPortInfo->PortNum)==-1)
        return;
    m_Inited=true;
}

ST_VOID PortCom::Uninit()
{
    if(!m_Inited)
        return ;
#ifdef _WIN32
    CloseHandle(m_ovRead.hEvent);
    CloseHandle(m_ovWrite.hEvent);
#endif
    Close();
    m_Inited=false;
}

ST_VOID PortCom::Open()
{
    if(m_IsOpened)
        return;
    m_Mutex.Lock();
#ifdef _WIN32
    com_fd = CreateFile(comName,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
    if(com_fd == INVALID_HANDLE_VALUE)
    {
        m_pLogger->LogDebug("com%d open error:errno=%d.",m_pPortInfo->PortNum, GetLastError());
        goto exit;
    }
    DCB dcb =GetDCB();
    COMMTIMEOUTS timeout =GetCOMMTIMEOUTS();
    if(!SetCommState(com_fd, &dcb) ||!SetupComm(com_fd, 2048, 2048) ||!SetCommTimeouts(com_fd, &timeout) ||
            !SetCommMask(com_fd, EV_RXCHAR | EV_TXEMPTY | EV_ERR) ||!PurgeComm(com_fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ))
    {
        m_pLogger->LogDebug("com%d setting error:errno=%d.",m_pPortInfo->PortNum, GetLastError());
        goto exit;
    }
    memset(&m_eov,0,sizeof(OVERLAPPED)) ;
    m_eov.hEvent = CreateEvent(	NULL,FALSE,FALSE,NULL);
#else
    ST_INT ret;
    com_fd=open(comName,O_RDWR | O_NOCTTY | O_NONBLOCK);
    m_pLogger->LogDebug("comName====%s",comName);
    if(com_fd==-1)
    {
        m_pLogger->LogDebug("com%d open error:errno=%d,descript=%s.",m_pPortInfo->PortNum, errno,strerror(errno));
        return;
    }
    fcntl(com_fd,F_SETFL,0);
    isatty(STDIN_FILENO);
    ret=Set_Speed(com_fd,m_pPortInfo->BaudRate);
    if(ret==-1)
    {
        m_pLogger->LogDebug("Set_Speedt=%d error",m_pPortInfo->BaudRate);
        return;
    }
    ret=Set_Parity(com_fd,m_pPortInfo->DataBits,m_pPortInfo->StopBits,m_pPortInfo->Parity);
    if(ret==-1)
    {
        m_pLogger->LogDebug("m_pPortInfo->DataBits,m_pPortInfo->StopBits,m_pPortInfo->Parity error:%d,%d,%d",m_pPortInfo->DataBits,m_pPortInfo->StopBits,m_pPortInfo->Parity);
        goto exit;
    }
#endif
    m_pLogger->LogDebug("com%d open success.",m_pPortInfo->PortNum);
    m_IsOpened=true;
    m_pChannel->GetCEngine()->OnConnect(m_pPortInfo->PortNum,m_pPortInfo->PortNum);
exit:
    m_Mutex.UnLock();
}

ST_VOID PortCom::Close()
{
#ifdef _WIN32
    if(com_fd != INVALID_HANDLE_VALUE)
    {
        SetCommMask(com_fd,0);
        EscapeCommFunction(com_fd,CLRDTR);
        PurgeComm(com_fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
        CloseHandle(com_fd);
        com_fd=INVALID_HANDLE_VALUE;
#else
    if(-1!=com_fd)
    {
        close(com_fd);
        com_fd=-1;
#endif
    }
    m_pChannel->GetCEngine()->OnDisconnect(m_pPortInfo->PortNum,m_pPortInfo->PortNum);
    m_IsOpened=false;
}

ST_BOOLEAN PortCom::IsOpened()
{
    return m_IsOpened;
}

ST_VOID PortCom::Recv()
{
    if(!m_IsOpened)
    {
        Thread::SLEEP(1000);
        return;
    }
#ifdef _WIN32
    dwEventMask = 0;
    ClearCommError(com_fd, &dwErrWord, &comstat);
    if(comstat.cbInQue != 0)
    {
        ReadToBuffer(comstat);
        return;
    }
    WaitCommEvent(com_fd, &dwEventMask, &m_eov);
    ::WaitForSingleObject(m_eov.hEvent, 1000);
    ClearCommError(com_fd, &dwErrWord, &comstat);
    if(dwEventMask & EV_RXCHAR && comstat.cbInQue != 0)
    {
        ReadToBuffer(comstat);
    }
#else
    FD_ZERO(&m_ReadSet);
    FD_SET(com_fd,&m_ReadSet);
    if(select(com_fd+1,&m_ReadSet,NULL,NULL,NULL)>0)
    {
        if(FD_ISSET(com_fd, &m_ReadSet))
        {
            ST_INT readLen=read(com_fd,m_PortBuf,1024);
            if(readLen > 0)
            {
                m_portTask.Write(m_PortBuf,readLen);
                m_portTask.PortDstAddr= 0;//m_pPortInfo->PortNum;
                m_portTask.LocalChannelID=m_pChannel->GetLocalChannelID();
                m_pChannel->GetCEngine()->ReadTask(&m_portTask);


                return;
            }
            else if(readLen < 0 && (errno==EAGAIN || errno==EINTR))
            {
                return;
            }
            else
            {
                m_pLogger->LogDebug("com%d recv error,errno:%d,desc:%s",m_pPortInfo->PortNum,errno,strerror(errno));
            }
        }
        else
        {
            return;
        }
    }
    m_pLogger->LogDebug("com%d select error, errno:%d, desc:%s",m_pPortInfo->PortNum, errno, strerror(errno));
#endif
}


ST_BOOLEAN PortCom::Send(ST_UINT64 portAddr,ST_BYTE *buf,ST_UINT size)
{
    ST_INT nLeft=size,sendLen=0;
    if(IsOpened())
    {
#ifdef _WIN32
        DWORD dwBytesSended = 0;
        ST_BOOLEAN wRes;
        wRes = WriteFile(com_fd, buf, size, &dwBytesSended, &m_ovWrite);
        if(!wRes)
        {
            if(GetLastError() == ERROR_IO_PENDING)
            {
                ::WaitForSingleObject(m_ovWrite.hEvent, 1000);
                wRes = GetOverlappedResult(com_fd, &m_ovWrite,&dwBytesSended,false);
                if(dwBytesSended != size)
                {
                    wRes = SD_FALSE;
                }
            }
            else
            {
                wRes = SD_FALSE;
            }
        }
        return wRes;
#else
        while(nLeft>0)
        {
            sendLen=write(com_fd,buf,size);
            if(sendLen>0)
            {
                buf+=sendLen;
                nLeft-=sendLen;
            }
            else
            {
                m_pLogger->LogDebug("com write error, errno:%d, desc:%s",errno,strerror(errno));
                return false;
            }
        }
        return true;
#endif
    }
    else
    {
        m_pLogger->LogDebug("com write error,m_IsOpened=false.");
        return false;
    }
}

ST_INT PortCom::Set_Com(ST_INT com)
{
    ST_CHAR strCom[10];
    Int2Str(com,strCom);
    Memset(comName,0,65);
#ifdef _WIN32
    Strcat(comName,PORTCOM);
    Strcat(comName,strCom);
#else
    Strcpy(comName,PORTCOM);
    Strcat(comName,strCom);
#endif
    return 1;
}

#ifndef _WIN32
ST_INT PortCom::Set_Parity(ST_INT fd, ST_INT databits,ST_INT stopbits,ST_INT parity)
{
    if(tcgetattr(fd, &comOpt) != 0)
    {
        m_pLogger->LogDebug("com tcgetattr() error------------------------");
        return -1;
    }
    comOpt.c_cflag |= (CLOCAL | CREAD);
    comOpt.c_cflag &= ~CSIZE;
    switch(databits)
    {
    case 8:
        comOpt.c_cflag |= CS8;
        break;
    case 7:
        comOpt.c_cflag |= CS7;
        break;
    default:
        return -1;
    }

    switch(parity)
    {
    case 0:
        comOpt.c_cflag &= ~PARENB;
        comOpt.c_iflag &= ~INPCK;
        break;
    case 1:
        comOpt.c_cflag |= PARENB;
        comOpt.c_cflag |= PARODD;
        comOpt.c_iflag |= INPCK;
        break;
    case 2:
        comOpt.c_cflag |= PARENB;
        comOpt.c_cflag &= ~PARODD;
        comOpt.c_iflag |= INPCK;
        break;
    case 3:
        comOpt.c_cflag |=PARENB|CMSPAR|PARODD;
        break;
    case 4:
        comOpt.c_cflag &= ~PARENB;
        break;
    default:
        return -1;
    }

    switch(stopbits)
    {
    case 0:
        comOpt.c_cflag &= ~CSTOPB;
        break;
    case 1:
        comOpt.c_cflag |= CSTOPB;
        break;
    default:
        return -1;
    }

    comOpt.c_cflag |= (CLOCAL | CREAD);
    comOpt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    comOpt.c_oflag &= ~OPOST;
    comOpt.c_oflag &= ~(ONLCR | OCRNL);

    comOpt.c_iflag &= ~(ICRNL | INLCR);
    comOpt.c_iflag &= ~(IXON | IXOFF | IXANY);

    tcflush(fd, TCIFLUSH);
    comOpt.c_cc[VTIME] = 0;
    comOpt.c_cc[VMIN] = 0;
    if(tcsetattr(fd, TCSANOW, &comOpt) != 0)
    {
        m_pLogger->LogDebug("com tcsetattr() error------------------------");
        return -1;
    }
    return 0;
}

ST_INT PortCom::Set_Speed(ST_INT fd, ST_INT speed)
{
    if(tcgetattr(fd, &comOpt) != 0)
        return -1;
    ST_INT speed_arr=0;
    switch(speed)
    {
    case 230400:
        speed_arr=B230400;
        break;
    case 115200:
        speed_arr=B115200;
        break;
    case 57600:
        speed_arr=B57600;
        break;
    case 38400:
        speed_arr=B38400;
        break;
    case 19200:
        speed_arr=B19200;
        break;
    case 9600:
        speed_arr=B9600;
        break;
    case 4800:
        speed_arr=B4800;
        break;
    case 2400:
        speed_arr=B2400;
        break;
    case 1200:
        speed_arr=B1200;
        break;
    default:
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    cfsetispeed(&comOpt, speed_arr);
    cfsetospeed(&comOpt, speed_arr);
    if(tcsetattr(fd, TCSANOW, &comOpt) != 0)
        return -1;
    tcflush(fd, TCIOFLUSH);
    return 0;
}
#endif

#ifdef _WIN32
DCB PortCom::GetDCB()
{
    DCB        dcb;
    dcb.DCBlength	= sizeof( DCB );
    GetCommState(com_fd, &dcb);
    dcb.BaudRate	=m_pPortInfo->BaudRate;
    dcb.ByteSize	=m_pPortInfo->DataBits;
    dcb.Parity		=m_pPortInfo->Parity;
    switch(m_pPortInfo->StopBits)
    {
    case 1:
        dcb.StopBits	=0;
        break;
    case 2:
        dcb.StopBits	=1;
        break;
    default:
        dcb.StopBits	=2;
    }
    dcb.fInX = dcb.fOutX = 0;
    dcb.XonChar = ASCII_XON;
    dcb.XoffChar = ASCII_XOFF;
    dcb.XonLim = 100 ;
    dcb.XoffLim = 100 ;
    dcb.fBinary = TRUE ;
    dcb.fParity = FALSE ;
    dcb.fAbortOnError = FALSE;
    return dcb;
}

COMMTIMEOUTS PortCom::GetCOMMTIMEOUTS()
{
    COMMTIMEOUTS timeout;
    Memset(&timeout,0,sizeof(COMMTIMEOUTS));
    timeout.ReadIntervalTimeout			= 5000*1000 /m_pPortInfo->BaudRate;
    timeout.ReadTotalTimeoutConstant	= 0;
    timeout.ReadTotalTimeoutMultiplier	= timeout.ReadIntervalTimeout;
    timeout.WriteTotalTimeoutConstant	= 0;
    timeout.WriteTotalTimeoutMultiplier= timeout.ReadIntervalTimeout;
    return timeout;
}


ST_VOID PortCom::ReadToBuffer(COMSTAT ComStat)
{
    BOOL       fReadStat ;
    DWORD      dwErrorFlags;
    DWORD      dwLength,len;
    DWORD      dwError;
    unsigned char	 abyInBuf[1024];
    dwLength = min( (DWORD) 1024, ComStat.cbInQue ) ;
    if (dwLength > 0)
    {
        fReadStat = ReadFile(com_fd, abyInBuf,dwLength, &len, &m_ovRead) ;
        if (!fReadStat)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                while(!GetOverlappedResult(com_fd, &m_ovRead, &len, TRUE ))
                {
                    dwError = GetLastError();
                    if(dwError == ERROR_IO_INCOMPLETE)
                        continue;
                    else
                    {
                        ClearCommError(com_fd, &dwErrorFlags, &ComStat ) ;
                        break;
                    }
                }
            }
            else
            {
                dwLength = 0 ;
                ClearCommError(com_fd, &dwErrorFlags, &ComStat ) ;
                return;
            }
        }
        m_portTask.Write(abyInBuf,len);
        m_portTask.PortDstAddr=-1;
        m_portTask.LocalChannelID=m_pChannel->GetLocalChannelID();
        m_pChannel->GetCEngine()->ReadTask(&m_portTask);
    }
}
#endif
