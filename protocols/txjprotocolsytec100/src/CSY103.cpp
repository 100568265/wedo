#include "CSY103.h"
#include "datetime.h"

const static ST_CHAR* szTYPE[] = {
	"XSYL111JK",
	"XSYT141JKK",
	"XSYC111JK",
	"XSYM111JK",
	"XSYA112"
};

ST_INT tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    ST_INT nsec;
    if( x->tv_sec > y->tv_sec )
        return   -1;
    if((x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec))
        return   -1;
    result->tv_sec = ( y->tv_sec-x->tv_sec );
    result->tv_usec = ( y->tv_usec-x->tv_usec );
    if(result->tv_usec<0)
    {
        result->tv_sec--;
        result->tv_usec+=1000000;
    }
    return   0;
}

CSY103::CSY103()
{
    //ctor
}

CSY103::~CSY103()
{
    //dtor
}


CSY103* CreateInstace()
{
    return new CSY103();
}

void	CSY103::Init()
{
	m_bFCB       = false;
	m_blink      = false;
	m_bcallclass1 = false;
	m_bresetlink = false;
	gettimeofday(&m_Timeout,0);
	m_bTask = false;
    time(&m_tlast);
}
void	CSY103::Uninit()
{

}
void CSY103::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	readed = 0;
	if(this->GetCurPort()!= NULL)
	{
		ST_INT	len = this->GetCurPort()->PickBytes(pbuf,5,300);
		if(len >= 5)
		{
			ST_BYTE frametype  = 0;
			ST_INT star;
			for(star = 0;star < len;star++)
			{
				if((pbuf[star] == 0x10) || (pbuf[star] == 0x68))
				{
					frametype = pbuf[star];
					break;
				}
			}
			if(star > 0)
			{
				//star大于0，说明有乱码， 把之前的乱码丢掉
				this->GetCurPort()->ReadBytes(pbuf,star);
//				this->GetCurPort()->Clear();
//				return;
			}
			if(star == len)
			{
				this->GetCurPort()->Clear();
				return;
			}
			// 找到头了，进行完成 头判断
			if(frametype == 0x10)
			{
				len = this->GetCurPort()->PickBytes(pbuf,5,300);
				if(len >= 5)
				{
					if(pbuf[0] == 0x10 && (pbuf[4] == 0x16))
					{
						readed  =  this->GetCurPort()->ReadBytes(pbuf,5);
						return;
					}
					else
					{
						ST_INT ierr;
						for(ierr = 1;ierr < 5;ierr++)
						{
							if(pbuf[ierr] == 0x10 || pbuf[ierr] == 0x68)
							{
								break;
							}
						}
						this->GetCurPort()->ReadBytes(pbuf,ierr);
//						this->OnShowMsg("短帧出现乱码，丢弃部分乱码",0);
					}
				}
				else
				{
//					this->OnShowMsg("数据长度不够一帧，继续接收",0);
				}
			}
			else if(frametype == 0x68)
			{
				len = this->GetCurPort()->PickBytes(pbuf,10,300);
				if(len >= 10)
				{
			        ST_INT naddr = this->GetDevice()->GetDeviceInfo()->Address;
			        if(((pbuf[0] == 0x68) && (pbuf[3] == 0x68) && (pbuf[1] == pbuf[2])) && (pbuf[5] == (ST_BYTE)naddr ))
					{
						//变长帧帧头判断通过
						ST_INT datalen = pbuf[1]+6;
						len = this->GetCurPort()->PickBytes(pbuf,datalen,300);
						if(len>=datalen)
						{
							ST_INT readedlen = this->GetCurPort()->ReadBytes(pbuf,datalen);
							if(readedlen != datalen)
							{
//								this->OnShowMsg("系统错误",0);
                                readed = 0;
                                this->GetCurPort()->Clear();
								return;
							}
							if(pbuf[readedlen-1] != 0x16)
							{
								readed = 0;
//								this->OnShowMsg("变长帧帧尾错误，系统将整个帧丢弃",0);
								this->GetCurPort()->Clear();
							}
							readed  =  readedlen;
						}
						else
						{
//							this->OnShowMsg("变长帧数据长度不够一帧，继续接收",0);
						}
					}
					else
					{
//						this->OnShowMsg("变长帧出现乱码，丢弃4个乱码",0);
						this->GetCurPort()->Clear();
					}
				}
				else
				{
//					this->OnShowMsg("数据长度不够帧头，继续接收",0);
                    this->GetCurPort()->Clear();
				}
			}
		}
	}
	//else
	//{
	//	m_bresetlink = false;
	//	m_bcallclass1 = false;
	//}
}

ST_BOOLEAN	CSY103::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

ST_BOOLEAN CSY103::OnSend()
{
    m_bTask = false;
    if(this->GetCurPort())
        this->GetCurPort()->Clear();
	if(this->HasTask())
	{
	    this->GetTask(&m_curTask);
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)
			{
				m_nroute  = m_curTask.taskAddr;
				m_byOnOff = (ST_BYTE)m_curTask.taskValue?0x82:0x81;
				YKSelect(m_nroute,m_byOnOff);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 1)
			{
				m_nroute  = m_curTask.taskAddr;
				m_byOnOff = (ST_BYTE)m_curTask.taskValue?0x02:0x01;
				YKExecut(m_nroute,m_byOnOff);
				m_bTask = true;
				return true;
			}
			else if(m_curTask.taskCmdCode == 2)
			{
				m_nroute  = m_curTask.taskAddr;
				m_byOnOff = (ST_BYTE)m_curTask.taskValue?0x42:0x41;
				YKCancel(m_nroute,m_byOnOff);
				m_bTask = true;
				return true;
			}
		}
		else if(!strcmp(m_curTask.taskCmd,"readfixvalue"))
		{
			if((m_curTask.taskCmdCode == 0))
			{
				ReadFixValue(m_curTask.taskAddr,m_curTask.taskAddr1);
				m_bTask = true;
				return true;
			}
		}
		else if(!strcmp(m_curTask.taskCmd,"writefixvalue"))
		{
			if((m_curTask.taskCmdCode == 0) || (m_curTask.taskCmdCode == 1))
			{
				WriteFixValue(m_curTask.taskAddr,m_curTask.taskAddr1,m_curTask);
				m_bTask = true;
				return true;
			}
		}
		else if(!strcmp(m_curTask.taskCmd,"readsystemfixvalue"))
		{
			if((m_curTask.taskCmdCode == 0))
			{
				ReadSystemFixValue(m_curTask.taskAddr,m_curTask.taskAddr1);
				m_bTask = true;
				return true;
			}
		}
		else if(!strcmp(m_curTask.taskCmd,"writesystemfixvalue"))
		{
			if((m_curTask.taskCmdCode == 0) || (m_curTask.taskCmdCode == 1))
			{
				WriteSystemFixValue(m_curTask.taskAddr,m_curTask.taskAddr1,m_curTask);
				m_bTask = true;
				return true;
			}
		}
	}
	else
	{
		time_t tnow = time(0);
        ST_DOUBLE interval = difftime(tnow, m_tlast);
		if(tnow < m_tlast || interval > 30*60)
		{
			SendTime();
			time(&m_tlast);
			Thread::SLEEP(20);
			return true;
		}
		else if(m_bcallclass1)
		{
			CallClass1Data();
			m_bcallclass1 = false;
			return true;
		}
		else
		{
			CallClass2Data();
			return true;
		}
	}
	return false;
}

ST_BOOLEAN CSY103::OnProcess(ST_BYTE* pbuf,ST_INT nlen)
{
	if(nlen==5)
	{
		m_bresetlink = true;
		return true;
	}
	else if (nlen > 10)
	{
		if(m_bTask)
		{
			bool breturn = false;
			if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
			{
				if((m_curTask.taskCmdCode == 0) || (m_curTask.taskCmdCode == 1) || (m_curTask.taskCmdCode == 2))
				{
					if((pbuf[10] == 0xff) && (pbuf[11] == 0xff) && (m_byOnOff==pbuf[12]))
					{
					  //  if(m_curTask.taskCmdCode == 0) sleep(1500);
					    m_curTask.taskResult.resultCode = 0;
					    m_curTask.isTransfer = 1;
					    Transfer(&m_curTask);
						memset(&m_curTask,0,sizeof(m_curTask));
						breturn = true;
					}
				}
			}
			else if(!strcmp(m_curTask.taskCmd,"readfixvalue")) //读定值
			{
				if(m_curTask.taskCmdCode == 0)
				{
					ExplainFixValue(pbuf,nlen);
					m_curTask.taskResult.resultCode=0;
                    m_curTask.isTransfer = 1;
                    Transfer(&m_curTask);
					memset(&m_curTask,0,sizeof(m_curTask));
					breturn = true;
				}
			}
			else if(!strcmp(m_curTask.taskCmd,"writefixvalue"))
			{
				if(((m_curTask.taskCmdCode == 0) && (pbuf[12] == 0x44)) || ((m_curTask.taskCmdCode == 1) && (pbuf[12] == 0x40)))
				{
					m_curTask.taskResult.resultCode=0;
                    m_curTask.isTransfer = 1;
                    Transfer(&m_curTask);
					memset(&m_curTask,0,sizeof(m_curTask));
					breturn = true;
				}
			}
			else if(!strcmp(m_curTask.taskCmd,"readsystemfixvalue")) //读定值
			{
				if(m_curTask.taskCmdCode == 0)
				{
					ExplainSystemFixValue(pbuf,nlen);
					m_curTask.taskResult.resultCode=0;
                    m_curTask.isTransfer = 1;
                    Transfer(&m_curTask);
					memset(&m_curTask,0,sizeof(m_curTask));
					breturn = true;
				}
			}
			else if(!strcmp(m_curTask.taskCmd,"writesystemfixvalue"))
			{
				if(((m_curTask.taskCmdCode == 0) && (pbuf[12] == 0x84)) || ((m_curTask.taskCmdCode == 1) && (pbuf[12] == 0x80)))
				{
					m_curTask.taskResult.resultCode=0;
					m_curTask.isTransfer =1;
					Transfer(&m_curTask);
					memset(&m_curTask,0,sizeof(m_curTask));
					breturn = true;
				}
			}
			return breturn;
		}
		else
		{
			if(pbuf[4]&0x20)
			{
				m_bcallclass1 = true;
			}
			if((pbuf[4]&0x0f) == 0x08)
			{
                if((pbuf[6]== 0x9)) //2级数据
				{
					ExplainClass2(pbuf,nlen);
				}
				else if(pbuf[6]== 0x28)//总召唤
				{
					ExplainCallAll(pbuf,nlen);
				}
				else if(pbuf[6]== 0x01)
				{
					ExplainEvent(pbuf,nlen);
				}
			}
		}
	}
	return true;
}

void  CSY103::ExplainSystemFixValue(ST_BYTE* pbuf,ST_INT nlen)
{
	memcpy(&m_curTask.taskResult.resultData[0],&pbuf[14],m_curTask.taskAddr1*2+12);
}

void  CSY103::ExplainFixValue(ST_BYTE* pbuf,ST_INT nlen)
{
	memcpy(&m_curTask.taskResult.resultData[0],&pbuf[14],m_curTask.taskAddr1*2);
}

void  CSY103::ExplainEvent(ST_BYTE* pbuf,ST_INT nlen)
{
    ST_INT nyxno = pbuf[11];
	ST_BYTE byStatu  = pbuf[12];
    ST_INT nIndexVarYX = 200;//事件遥信从200开始
    if((nyxno>=82) && (nyxno<=97)) nIndexVarYX = 100-82+24;
    this->UpdateValue(nIndexVarYX+nyxno,(ST_BYTE)byStatu); //1是分，2是合

    ProtocolTask *task;
    task = new ProtocolTask;
    task->isTransfer = true;
    task->transChannelId = -1;
    task->channelId = this->GetDevice()->GetDeviceInfo()->Channel;
    task->deviceId = this->GetDevice()->GetDeviceInfo()->DeviceId;
    Strcpy(task->taskCmd,"SOE");
    task->taskCmdCode = 0;
    task->taskParamLen = 14;
    task->taskAddr = nIndexVarYX+nyxno;
    task->taskValue = byStatu;
    task->taskAddr1 = this->GetDevice()->GetDeviceInfo()->DeviceId;
    task->taskParam[0] = (2000+pbuf[19]/16*10+pbuf[19]%16)&0xff;
    task->taskParam[1] = ((2000+pbuf[19]/16*10+pbuf[19]%16)&0xff00)>>8;
    task->taskParam[2] = pbuf[18]/16*10+pbuf[18]%16;
    task->taskParam[3] = pbuf[17]/16*10+pbuf[17]%16;
    task->taskParam[4] = pbuf[16]/16*10+pbuf[16]%16;
    task->taskParam[5] = pbuf[15]/16*10+pbuf[15]%16;
    task->taskParam[6] = (pbuf[13]+pbuf[14]*256)/1000;
    task->taskParam[7] = ((pbuf[13]+pbuf[14]*256)%1000)&0xff;
    task->taskParam[8] = (((pbuf[13]+pbuf[14]*256)%1000)&0xff00)>>8;
    task->taskParam[9] = byStatu;
    task->taskParam[10] = (ST_BYTE)(nIndexVarYX+nyxno);
    task->taskParam[11] = (ST_BYTE)((nIndexVarYX+nyxno)>>8);
    task->taskParam[12] = (ST_BYTE)(task->taskAddr1);
    task->taskParam[13] = (ST_BYTE)((task->taskAddr1)>>8);
    task->ignoreBack = 1;
    task->taskTime = 1000;
    Transfer(task);
}

void  CSY103::ExplainCallAll(ST_BYTE* pbuf,ST_INT nlen)
{
    ST_INT nIndexData = 12;
	ST_INT nIndexVarYX = 100;
	for(ST_INT i = 0;i<5;i++) //接着是5个字节的遥信 ，点从100开始
	{
		ST_BYTE by = 0x01;
		ST_BYTE byValue = 0x00;
		for(ST_INT j=0;j<8;j++)
		{
			byValue = ((by<<j)&pbuf[nIndexData++])?0x01:0x00;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
		}
	}
}

void  CSY103::ExplainClass2(ST_BYTE* pbuf,ST_INT nlen)
{
	ST_INT nType = -1;
	string stypename1 = this->GetDevice()->GetDeviceInfo()->Deviceserialtype;
    string stypename = "";
	for(ST_INT nn=0;nn<5;nn++)
	{
		stypename = szTYPE[nn];
		if(stypename == stypename1)
			nType = nn;
	}
	if(nType == -1) return;
	if(stypename1 == "XSYL111JK")
	{
		ST_INT nIndexVarYC = 0;
		ST_INT nIndexVarYX = 100;//遥信从100开始
		ST_INT nIndexData =12;//从12个字节开始
		ST_INT i=0;
		for(i = 0;i<30;i++)//有44个遥测
		{
			ST_FLOAT fvalue = 0.0;
			if(pbuf[i*2+nIndexData+1]&0x80)
				fvalue = -1*(0xffff - (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData]) +1)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			else
				fvalue = (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
            this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData = 12 + 60;//遥信从100个字节开始
		for(i = 0;i<5;i++) //接着是5个字节的遥信
		{
			ST_BYTE by = 0x01;
			ST_BYTE byValue = 0x00;
			for(ST_INT j=0;j<8;j++)
			{
				byValue = ((by<<j)&pbuf[nIndexData])?0x01:0x00;
				this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
			}
			nIndexData++;
		}

		this->UpdateValue(nIndexVarYX++,(ST_BYTE)(pbuf[nIndexData])); //系统参数  //1个字节
		nIndexData++;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		for(i=0;i<2;i++)
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_DOUBLE dvalue = (pbuf[i*4+nIndexData+3]*256*256*256 + pbuf[i*4+nIndexData+2]*256*256+pbuf[i*4+nIndexData+1]*256 + pbuf[i*4+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_DOUBLE)dvalue);
		}
		nIndexData +=8;
		{
			ST_FLOAT fValue = pbuf[nIndexData+1]*256+pbuf[nIndexData];
			ST_BYTE byValue = 0x00;
			if(fValue>=18000) byValue =1;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)(byValue)); //充电状态  //2个字节
		}
	}
	else if(stypename1 == "XSYT141JKK")
	{
		ST_INT nIndexVarYC = 0;
		ST_INT nIndexVarYX = 100;//遥信从100开始
		ST_INT nIndexData =12;//从12个字节开始
		ST_INT i=0;
		for(i = 0;i<30;i++)//有44个遥测
		{
			ST_FLOAT fvalue = 0.0;
			if(pbuf[i*2+nIndexData+1]&0x80)
				fvalue = -1*(0xffff - (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData]) +1)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			else
				fvalue = (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData = 12 + 60;//遥信从100个字节开始
		for(i = 0;i<5;i++) //接着是5个字节的遥信
		{
			ST_BYTE by = 0x01;
			ST_BYTE byValue = 0x00;
			for(ST_INT j=0;j<8;j++)
			{
				byValue = ((by<<j)&pbuf[nIndexData])?0x01:0x00;
				this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
			}
			nIndexData++;
		}
		this->UpdateValue(nIndexVarYX++,(ST_BYTE)(pbuf[nIndexData])); //系统参数  //1个字节
		nIndexData++;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		for(i=0;i<2;i++)
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_DOUBLE dvalue = (pbuf[i*4+nIndexData+3]*256*256*256 + pbuf[i*4+nIndexData+2]*256*256+pbuf[i*4+nIndexData+1]*256 + pbuf[i*4+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_DOUBLE)dvalue);
		}
		nIndexData +=8;
		{
			ST_FLOAT fValue = pbuf[nIndexData+1]*256+pbuf[nIndexData];
			ST_BYTE byValue = 0x00;
			if(fValue>=18000) byValue =1;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)(byValue)); //充电状态  //2个字节
		}
	}
	else if(stypename1 == "XSYC111JK")
	{
		ST_INT nIndexVarYC = 0;
		ST_INT nIndexVarYX = 100;//遥信从100开始
		int nIndexData =12;//从12个字节开始
		ST_INT i=0;
		for(i = 0;i<30;i++)//有44个遥测
		{
			ST_FLOAT fvalue = 0.0;
			if(pbuf[i*2+nIndexData+1]&0x80)
				fvalue = -1*(0xffff - (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData]) +1)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			else
				fvalue = (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData = 12 + 60;//遥信从100个字节开始
		for(i = 0;i<5;i++) //接着是5个字节的遥信
		{
			ST_BYTE by = 0x01;
			ST_BYTE byValue = 0x00;
			for(ST_INT j=0;j<8;j++)
			{
				byValue = ((by<<j)&pbuf[nIndexData])?0x01:0x00;
				this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
			}
			nIndexData++;
		}
		this->UpdateValue(nIndexVarYX++,(ST_BYTE)(pbuf[nIndexData])); //系统参数  //1个字节
		nIndexData++;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		for(i=0;i<2;i++)
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_DOUBLE dvalue = (pbuf[i*4+nIndexData+3]*256*256*256 + pbuf[i*4+nIndexData+2]*256*256+pbuf[i*4+nIndexData+1]*256 + pbuf[i*4+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_DOUBLE)dvalue);
		}
		nIndexData +=8;
		{
			ST_FLOAT fValue = pbuf[nIndexData+1]*256+pbuf[nIndexData];
			ST_BYTE byValue = 0x00;
			if(fValue>=18000) byValue =1;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)(byValue)); //充电状态  //2个字节
		}
	}
	else if(stypename1 == "XSYM111JK")
	{
		ST_INT nIndexVarYC = 0;
		ST_INT nIndexVarYX = 100;//遥信从100开始
		ST_INT nIndexData =12;//从12个字节开始
		ST_INT i=0;
		for(i = 0;i<30;i++)//有44个遥测
		{
			ST_FLOAT fvalue = 0.0;
			if(pbuf[i*2+nIndexData+1]&0x80)
				fvalue = -1*(0xffff - (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData]) +1)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			else
				fvalue = (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData = 12 + 60;//遥信从100个字节开始
		for(i = 0;i<5;i++) //接着是5个字节的遥信
		{
			ST_BYTE by = 0x01;
			ST_BYTE byValue = 0x00;
			for(ST_INT j=0;j<8;j++)
			{
				byValue = ((by<<j)&pbuf[nIndexData])?0x01:0x00;
				this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
			}
			nIndexData++;
		}
		this->UpdateValue(nIndexVarYX++,(ST_BYTE)(pbuf[nIndexData])); //系统参数  //1个字节
		nIndexData++;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		for(i=0;i<2;i++)
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_DOUBLE dvalue = (pbuf[i*4+nIndexData+3]*256*256*256 + pbuf[i*4+nIndexData+2]*256*256+pbuf[i*4+nIndexData+1]*256 + pbuf[i*4+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_DOUBLE)dvalue);
		}
		nIndexData +=8;
		{
			ST_FLOAT fValue = pbuf[nIndexData+1]*256+pbuf[nIndexData];
			ST_BYTE byValue = 0x00;
			if(fValue>=18000) byValue =1;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)(byValue)); //充电状态  //2个字节
		}
	}
	else if(stypename1 == "XSYA112")
	{
		ST_INT nIndexVarYC = 0;
		ST_INT nIndexVarYX = 100;//遥信从100开始
		ST_INT nIndexData =12;//从12个字节开始
		ST_INT i=0;
		for(i = 0;i<30;i++)//有44个遥测
		{
			ST_FLOAT fvalue = 0.0;
			if(pbuf[i*2+nIndexData+1]&0x80)
				fvalue = -1*(0xffff - (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData]) +1)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			else
				fvalue = (pbuf[i*2+nIndexData+1]*256 + pbuf[i*2+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData = 12 + 60;//遥信从100个字节开始
		for(i = 0;i<5;i++) //接着是5个字节的遥信
		{
			ST_BYTE by = 0x01;
			ST_BYTE byValue = 0x00;
			for(int j=0;j<8;j++)
			{
				byValue = ((by<<j)&pbuf[nIndexData])?0x01:0x00;
				this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
			}
			nIndexData++;
		}
		this->UpdateValue(nIndexVarYX++,(ST_BYTE)(pbuf[nIndexData])); //系统参数  //1个字节
		nIndexData++;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_FLOAT fvalue = (pbuf[nIndexData+1]*256 + pbuf[nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
		nIndexData +=2;
		for(i=0;i<2;i++)
		{
			if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0) this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient=1;
			ST_DOUBLE dvalue = (pbuf[i*4+nIndexData+3]*256*256*256 + pbuf[i*4+nIndexData+2]*256*256+pbuf[i*4+nIndexData+1]*256 + pbuf[i*4+nIndexData])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_DOUBLE)dvalue);
		}
		nIndexData +=8;
		{
			ST_FLOAT fValue = pbuf[nIndexData+1]*256+pbuf[nIndexData];
			ST_BYTE byValue = 0x00;
			if(fValue>=18000) byValue =1;
			this->UpdateValue(nIndexVarYX++,(ST_BYTE)(byValue)); //充电状态  //2个字节
		}
	}
	else return;
}

//复位链路
void CSY103::LinkReset(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = 0;
	linkcode.FCV = 0;
	linkcode.FUN = 0x07;
	byBuf[0] = 0x10;
	byBuf[1] = *(ST_BYTE*)&linkcode;
	byBuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[3] = GetCHKSUM(&byBuf[1],2);
	byBuf[4] = 0x16;
	this->Send(byBuf,5);
}

//总召唤开始
//标准103:68 09	09 68 53/73	ADDR 64(100) 01	06激活 ADDR	00 00 14(20) CS	16
void CSY103:: CallAll(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 0x09;
	byBuf[2] = 0x09;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x07;//100???
	byBuf[7] = 0x81;
	byBuf[8] = 0x09;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xff;
	byBuf[11] = 0x00;
	byBuf[12] = 0x00;
	byBuf[13] = GetCHKSUM(&byBuf[4],9);
	byBuf[14] = 0x16;
	this->Send(byBuf,15);
}

//召唤一级数据
void CSY103::CallClass1Data(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;;
	linkcode.FCV = 1;
	linkcode.FUN = 0x0a;
	m_bFCB = !m_bFCB;
	byBuf[0] = 0x10;
	byBuf[1] = *(ST_BYTE*)&linkcode;
	byBuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[3] = GetCHKSUM(&byBuf[1],2);
	byBuf[4] = 0x16;
	this->Send(byBuf,5);
}

//召唤二级数据
void CSY103::CallClass2Data(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;;
	linkcode.FCV = 1;
	linkcode.FUN = 0x0b;
	m_bFCB = !m_bFCB;
	byBuf[0] = 0x10;
	byBuf[1] = *(ST_BYTE*)&linkcode;
	byBuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[3] = GetCHKSUM(&byBuf[1],2);
	byBuf[4] = 0x16;
	this->Send(byBuf,5);
}

//对时
void CSY103::SendTime(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
	m_bFCB = !m_bFCB;
	byBuf[0] = 0x68;
	byBuf[1] = 0x0f;
	byBuf[2] = 0x0f;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x06;
	byBuf[7] = 0x81;
	byBuf[8] = 0x88;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xff;
	byBuf[11] = 0x00;

	time_t t_now = time(0);
	if (t_now < 0) return;
    struct tm tm_now;
    DateTime::localtime (t_now, tm_now);

	byBuf[12] =  (tm_now.tm_sec * 1000) % 256;
	byBuf[13] =  (tm_now.tm_sec * 1000) / 256;
    byBuf[14] = ( tm_now.tm_min / 10) * 16   + ( tm_now.tm_min % 10);
	byBuf[15] = ( tm_now.tm_hour/ 10) * 16   + ( tm_now.tm_hour% 10);
	byBuf[16] = ( tm_now.tm_mday/ 10) * 16   + ( tm_now.tm_mday% 10);
    byBuf[17] = ((tm_now.tm_mon + 1) /10)*16 + ((tm_now.tm_mon + 1) %10);
    byBuf[18] = ((tm_now.tm_year-100)/10)*16 + ((tm_now.tm_year-100)%10);

	byBuf[19] = GetCHKSUM(&byBuf[4], 15);
	byBuf[20] = 0x16;
	this->Send(byBuf, 21);
}

void CSY103::Resetdevice(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 10;
	byBuf[2] = 10;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x14;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xe0;
	byBuf[11] = 0x13;
	byBuf[12] = 0x02;
	byBuf[13] = 0xee;
	byBuf[14] = GetCHKSUM(&byBuf[4],10);
	byBuf[15] = 0x16;
	this->Send(byBuf,16);
}

void  CSY103::ReStart(void)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = 0;
	linkcode.FCV = 0;
	linkcode.FUN = 0x00;
	byBuf[0] = 0x10;
	byBuf[1] = *(ST_BYTE*)&linkcode;
	byBuf[2] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[3] = GetCHKSUM(&byBuf[1],2);
	byBuf[4] = 0x16;
	this->Send(byBuf,5);
}

ST_BOOLEAN CSY103::YKSelect(ST_INT nroute,ST_BYTE byOnOff)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 10;
	byBuf[2] = 10;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x14;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xff;
	byBuf[11] = 0xff;
	byBuf[12] = byOnOff;
	byBuf[13] = 0xee;
	byBuf[14] = GetCHKSUM(&byBuf[4],10);
	byBuf[15] = 0x16;
	this->Send(byBuf,16);
	return true;
}

ST_BOOLEAN CSY103::YKExecut(ST_INT nroute,ST_BYTE byOnOff)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 10;
	byBuf[2] = 10;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x14;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xff;
	byBuf[11] = 0xff;
	byBuf[12] = byOnOff;
	byBuf[13] = 0xee;
	byBuf[14] = GetCHKSUM(&byBuf[4],10);
	byBuf[15] = 0x16;
	this->Send(byBuf,16);
	return true;
}

ST_BOOLEAN CSY103::YKCancel(ST_INT nroute,ST_BYTE byOnOff)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 10;
	byBuf[2] = 10;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x14;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 0xff;
	byBuf[11] = 0xff;
	byBuf[12] = byOnOff;
	byBuf[13] = 0xee;
	byBuf[14] = GetCHKSUM(&byBuf[4],10);
	byBuf[15] = 0x16;
	this->Send(byBuf,16);
	return true;
}

ST_BOOLEAN CSY103::ReadFixValue(ST_INT nfixIndex,ST_INT nfixnum)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 11;
	byBuf[2] = 11;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x0a;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 254;
	byBuf[11] = nfixnum;
	byBuf[12] = 0x42;
	byBuf[13] = nfixIndex;
    byBuf[14] = 0xee;
	byBuf[15] = GetCHKSUM(&byBuf[4],11);
	byBuf[16] = 0x16;
	this->Send(byBuf,17);
	return true;
}

ST_BOOLEAN CSY103::WriteFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = nfixnum*2+11;
	byBuf[2] = nfixnum*2+11;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x0a;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 254;
	byBuf[11] = nfixnum;
	byBuf[12] = 0x44;
	byBuf[13] = nfixIndex;
	if(pTask.taskCmdCode == 0)
	{
		for(ST_INT i=0;i<nfixnum*2;i++)
			byBuf[i+14] = ~pTask.taskParam[i];
	}
	else if(pTask.taskCmdCode == 1)
	{
		for(ST_INT i=0;i<nfixnum*2;i++)
			byBuf[i+14] = pTask.taskParam[i];
		byBuf[12] = 0x40;
	}
    byBuf[14+nfixnum*2] = 0xee;
	byBuf[15+nfixnum*2] = GetCHKSUM(&byBuf[4],nfixnum*2+11);
	byBuf[16+nfixnum*2] = 0x16;
	this->Send(byBuf,nfixnum*2+17);
	return true;
}
//68 0b 0b 68 73 01 0a 81 14 01 fc 26 82 16 ee bc 16
ST_BOOLEAN CSY103::ReadSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = 11;
	byBuf[2] = 11;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x0a;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 252;
	byBuf[11] = nfixnum;
	byBuf[12] = 0x82;
	byBuf[13] = nfixIndex;
    byBuf[14] = 0xee;
	byBuf[15] = GetCHKSUM(&byBuf[4],11);
	byBuf[16] = 0x16;
	this->Send(byBuf,17);
	return true;
}

ST_BOOLEAN CSY103::WriteSystemFixValue(ST_INT nfixIndex,ST_INT nfixnum,ProtocolTask pTask)
{
	ST_BYTE byBuf[1024];
	SENDLINKCODE linkcode;
	linkcode.DIR = 0;
	linkcode.PRM = 1;
	linkcode.FCB = m_bFCB?1:0;
	linkcode.FCV = 1;
	linkcode.FUN = 0x03;
    m_bFCB = !m_bFCB;
    byBuf[0] = 0x68;
	byBuf[1] = nfixnum*2+11+12;
	byBuf[2] = nfixnum*2+11+12;
	byBuf[3] = 0x68;
	byBuf[4] = *(ST_BYTE*)&linkcode;
	byBuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[6] = 0x0a;
	byBuf[7] = 0x81;
	byBuf[8] = 0x14;
	byBuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	byBuf[10] = 252;
	byBuf[11] = nfixnum;
	byBuf[12] = 0x84;
	byBuf[13] = nfixIndex;
	if(pTask.taskCmdCode == 0)
	{
		for(ST_INT i=0;i<nfixnum*2+12;i++)
			byBuf[i+14] = ~pTask.taskParam[i];
	}
	else if(pTask.taskCmdCode == 1)
	{
		for(ST_INT i=0;i<nfixnum*2+12;i++)
			byBuf[i+14] = pTask.taskParam[i];
		byBuf[12] = 0x80;
	}
    byBuf[14+nfixnum*2+12] = 0xee;
	byBuf[15+nfixnum*2+12] = GetCHKSUM(&byBuf[4],nfixnum*2+11+12);
	byBuf[16+nfixnum*2+12] = 0x16;
	this->Send(byBuf,nfixnum*2+17+12);
	return true;
}

//校验
ST_BYTE CSY103::GetCHKSUM(ST_BYTE* pbuf,ST_BYTE bylen)
{
	ST_BYTE byRet = 0x00;
	for(ST_BYTE b=0; pbuf&&(b<bylen); b++)
		byRet += pbuf[b];
	return byRet;
}

