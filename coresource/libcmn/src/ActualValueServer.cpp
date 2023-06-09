//#include "stdafx.h"
#include "ActualValueServer.h"
#include "syslogger.h"
//#include "sigaction.h"
extern NodeTree *g_pTree;

ActualValueServer::ActualValueServer()
{
	m_port=NULL;
	m_init=false;
	m_working=false;
}

ActualValueServer::~ActualValueServer()
{
	Uninit();
}

ST_VOID ActualValueServer::Init()
{
	if(!m_init) {
		m_PortInfo.LocalPort = 9998;
		Strcpy(m_PortInfo.LocalAddress, "0.0.0.0");
		Strcpy(m_PortInfo.PortName, "actual");
		m_PortInfo.PortType = PortBase::PORT_TCPSERVER;
		m_port = new PortTcpServerBase(NULL, &m_PortInfo);
		m_port->Init();
/*		#ifdef _WIN32
        #else
            struct sigaction sa;
            sa.sa_handler = SIG_IGN;
            sa.sa_flags=0;
            sigaction(SIGPIPE, &sa, 0 );
        #endif*/
		m_init=true;
	}
}

ST_VOID	ActualValueServer::Uninit()
{
	if(NULL != m_port) {
		m_port->Close();
		m_thread.Stop(1000);
		m_sendThread.Stop(1000);
		delete m_port;
		m_port = NULL;
	}
	m_init = false;
	m_working = false;
}

ST_VOID ActualValueServer::Work()
{
	if(!m_working){
		m_port->Open();
		m_thread.Start(ReceiveProc,m_port,true);
		m_sendThread.Start(SendProc,this,true);
		m_working=true;
	}
}

#ifdef _WIN32
ST_UINT32 ActualValueServer::ReceiveProc(ST_VOID *param)
#else
ST_VOID *ActualValueServer::ReceiveProc(ST_VOID *param)
#endif
{
    PortBase *pPort=(PortBase*)param;
	if(pPort==NULL) return 0;
    pPort->Recv();
	return 0;
}

ST_VOID *ActualValueServer::SendProc(ST_VOID *param)
{
    ActualValueServer *m_actual=(ActualValueServer*)param;
	if(m_actual==NULL) return 0;
    m_actual->protocolMsg();
	return 0;
}

ST_VOID ActualValueServer::SendMessage(const ST_CHAR *msg, ST_INT dataLen, ST_INT deviceId)
{

//	int len = Strlen(msg);
	if(m_port && m_port->IsOpened() && dataLen > 0)
	{
		ST_CHAR *tmp = new ST_CHAR[dataLen];
		Memcpy(tmp, msg, dataLen);

		m_port->Send(-1, (ST_BYTE*)tmp, dataLen);
		delete[] tmp;
        Thread::SLEEP(100);
	}
/*	else{
		Thread::SLEEP(100);
	}*/
}

ST_VOID ActualValueServer::protocolMsg()
{
    Thread::SLEEP(1000); //
    ST_INT j = 0;
    ST_BYTE bySendbuf[1024];
    ST_INT  nDataLen = 0;
	ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);
	for(ST_INT i = 0; i < devcount; ++i)
	{
		if (g_pTree->GetNodeName("", i, devname) < 0)
            continue;


        /*if(strcmp(devname,"channelstate")==0|strcmp(devname,"devicestate")==0)
            continue;*/

		ST_INT varcount = g_pTree->GetNameNodeCount(devname);
        if (varcount <= 0)
            continue;

        //SysLogger::GetInstance()->LogInfo("dev name:%s,dev varcount:%d",devname,varcount);
		for(j = 0; j < varcount; ++j)
		{
//            memset(fullname, 0, sizeof(fullname) / 2);
//            memset(ditname , 0, sizeof(ditname ) / 2);
            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

		    g_pTree->GetNodeName(devname, j, ditname);

            strcpy(fullname, devname); strcat(fullname, ".");
		    strcat(fullname, ditname); strcat(fullname, ".value");
		    ST_DUADDR tdd;
		    g_pTree->GetNodeAddr(fullname, tdd);
            ST_VARIANT vValue;
            // GetVariableValueByName(&fullname[0],vValue);
            GetVariableValueByAddr(tdd, vValue);

            if(tdd.type == -2) continue;
            bySendbuf[0] = 0xaa;
            bySendbuf[1] = 0xbb;
            bySendbuf[2] = 0x01;
            bySendbuf[5] =  tdd.connect& 0xff;
            bySendbuf[6] = (tdd.connect& 0xff00)>>8;
            bySendbuf[7] =  tdd.device & 0xff;
			bySendbuf[8] = (tdd.device & 0xff00)>>8;
			bySendbuf[9+nDataLen]  =  tdd.addr & 0xff;
            bySendbuf[10+nDataLen] = (tdd.addr & 0xff00)>>8;
			bySendbuf[11+nDataLen] = vValue.vt;
            bySendbuf[12+nDataLen] = tdd.type;
            if(tdd.type == -1)
            {
                bySendbuf[2] = 0x03;
                bySendbuf[12+nDataLen] = 254;
            }
            switch(vValue.vt)
            {
                case VALType_SByte:
                {
                    ST_BYTE Value = vValue.bVal;
                    bySendbuf[13+nDataLen] = Value;
                    nDataLen = nDataLen+5;
                }
                break;
            case VALType_Byte:
                {
                    ST_BYTE Value = vValue.bVal;
                    bySendbuf[13+nDataLen] = Value;
                    nDataLen = nDataLen+5;

                }
                break;
            case VALType_Boolean:
                {
                    ST_BYTE Value = vValue.blVal;
                    bySendbuf[13+nDataLen] = Value;
                    nDataLen = nDataLen+5;

                }
                break;
            case VALType_Int16:
                {
                    ST_INT Value  = vValue.sVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,2);
                    nDataLen = nDataLen+6;
                }
                break;
            case VALType_UInt16:
                {
                    ST_INT Value  = vValue.usVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,2);
                    nDataLen = nDataLen+6;
                }
                break;
            case VALType_Int32:
                {
                    ST_INT Value  = vValue.iVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,4);
                    nDataLen = nDataLen+8;
                }
                break;
            case VALType_UInt32:
                {
                    ST_INT Value  = vValue.uiVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,4);
                    nDataLen = nDataLen+8;
                }
                break;
            case VALType_Float:
                {
                    ST_FLOAT Value  = vValue.fVal;
                    memcpy(&bySendbuf[13 + nDataLen], &Value, 4);
                    nDataLen = nDataLen + 8;
                }
                break;
            case VALType_Int64:
                {
                    ST_LONG Value  = vValue.lVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,8);
                    nDataLen = nDataLen+12;

                }
                break;
            case VALType_UInt64:
                {
                    ST_LONG Value  = vValue.ulVal;
                    memcpy(&bySendbuf[13+nDataLen],&Value,8);
                    nDataLen = nDataLen+12;

                }
                break;
            case VALType_Double:
                {
                    ST_DOUBLE Value  = vValue.dtVal;
                    memcpy(&bySendbuf[13 + nDataLen], &Value, 8);
                    nDataLen = nDataLen + 12;

                }
                break;
            default:
                break;
            }
            if (nDataLen >= 300) {
				nDataLen += 4;
				bySendbuf[3] =  nDataLen & 0xff;
				bySendbuf[4] = (nDataLen & 0xff00) >> 8;
				//m_port->Send(-1, (ST_BYTE*)bySendbuf, nDataLen + 5);
				//this->Send(bySendbuf, nDataLen + 5);
                this->SendMessage((ST_CHAR*)bySendbuf,nDataLen + 5,0);
                //SysLogger::GetInstance()->LogInfo("Actual value1 : %s",bySendbuf);
				Thread::SLEEP(500);
                nDataLen = 0;
			}
		}
		if (nDataLen > 0)
        {
            nDataLen += 4;
            bySendbuf[3] =  nDataLen & 0xff;
            bySendbuf[4] = (nDataLen & 0xff00) >> 8;
            //m_port->Send(-1, (ST_BYTE*)bySendbuf, nDataLen + 5);
            //this->SendData((ST_CHAR*)bySendbuf);
            this->SendMessage((ST_CHAR*)bySendbuf,nDataLen + 5,0);
            //SysLogger::GetInstance()->LogInfo("Actual value2 : %s",(ST_BYTE*)bySendbuf);
            Thread::SLEEP(200);
        }
        nDataLen   = 0;
	}

}
