#include "XMLighting.h"
#include "Device.h"

static ST_BYTE CSCheck(ST_BYTE*pbuf,int len)
{
	ST_BYTE count = 0;
	for (int i=0;i<len;i++)
	{
		count = count + pbuf[i];
	}
	return count;
}

static ST_BYTE BcdToDec(ST_BYTE bcd)
{
	unsigned char a, b;

	a = (bcd >> 4);
	b = bcd & 0x0f;

	return (a * 10 + b);
}

XMLighting * CreateInstace()
{
	return new XMLighting();
}

XMLighting::XMLighting()
{
    //ctor
    _counter = 0;
}

XMLighting::~XMLighting()
{
    //dtor
}

ST_BOOLEAN XMLighting::IsSupportEngine(ST_INT engineType)
{
	return true;
}

ST_VOID	    XMLighting::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
    readed = 0;
	if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 15, 2000);
		if(len < 12) {
			ShowMessage ("Insufficient data length");
			return;
		}
		ST_INT star = 0;
		for(; star < len; ++star) {
			if(pbuf[star] == 0x68)
				break;
		}
		if(len == star) {
			ShowMessage ("Garbled code, clear buffer.");
			this->GetCurPort()->Clear();
			return;
		}
		if(star > 0)
		{
			this->GetCurPort()->ReadBytes(pbuf, star);
		}
		len = this->GetCurPort()->PickBytes(pbuf, 15, 2000);
		if((pbuf[0] == 0x68) && (pbuf[9] == 0x68))
		{
			ST_INT ndatalen = pbuf[11] + pbuf[12]*256+15;
			if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) == ndatalen)
			{
 //               if (CSCheck(pbuf,ndatalen-2) == pbuf[ndatalen-2])
//				{
					readed = ndatalen;
                    return;
//				}
/*				else
                {
                    this->ShowMessage("check error!");
					return;
                }*/


			}
		}
		else
		{
			ShowMessage("Sync header error.");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
			return;
		}
	}
}

ST_BOOLEAN	XMLighting::OnSend()
{
    if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 0)
				SendYK(m_curTask.taskAddr,m_curTask.taskValue,false);
			else if(m_curTask.taskCmdCode == 1)
				SendYK(m_curTask.taskAddr,m_curTask.taskValue,false);
		}
		return true;
	}
    AskState();
    return true;
}


ST_BOOLEAN	XMLighting::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    switch(pbuf[10])
	{
	case 0xA1:{ // 照明查询应答
				Analysis21H(&pbuf[13]);
			  }break;
	case 0xA0:{	// 遥控返回
				m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                Memset(&m_curTask, 0, sizeof(m_curTask));
			  }break;
	default:break;
	}
    return true;
}
/*
void    XMLighting::getDevAddr()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

	m_addrarea[0] = (info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);
	m_addrarea[1] = (info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
	m_addrarea[2] = (info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
	m_addrarea[3] = (info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
	m_addrarea[4] = (info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
	m_addrarea[5] = (info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
}
*/

void XMLighting::AskState()
{
	ST_BYTE sendbuf[256] = {0};
	sendbuf[0]	= 0x68;
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	sendbuf[1]	= (info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);
	sendbuf[2]	= (info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
	sendbuf[3]	= (info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
	sendbuf[4]	= (info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
	sendbuf[5]	= (info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
	sendbuf[6]	= (info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);

	sendbuf[7]	= _counter % 256;
	sendbuf[8]	= _counter / 256;
	sendbuf[9]	= 0x68;
	sendbuf[10]	= 0x21;
	sendbuf[11]	= 0x00;
	sendbuf[12]	= 0x00;

	sendbuf[13]	= CSCheck(sendbuf,13);
	sendbuf[14]	= 0x16;
	this->Send(sendbuf,15);
	_counter++;
}

void	XMLighting::Analysis21H(ST_BYTE* pbuf)
{
    float value = BcdToDec(pbuf[0]) + BcdToDec(pbuf[1])*100;
	value = value /10;
	this->UpdateValue(0,value);
	for (int i=0;i<8;i++)
	{
		this->UpdateValue(i+1,pbuf[i+2]);
	}
	value = BcdToDec(pbuf[10]) + BcdToDec(pbuf[11])*100;
	this->UpdateValue(9,value/10);

	double dvalue = BcdToDec(pbuf[12]) + BcdToDec(pbuf[13])*100 + BcdToDec(pbuf[14])*10000 + BcdToDec(pbuf[15])*1000000;
	dvalue = dvalue / 10;
	this->UpdateValue(10,dvalue);

	value = BcdToDec(pbuf[16]) + BcdToDec(pbuf[17])*100;
	this->UpdateValue(11,value);
}


void XMLighting::SendYK(ST_UINT ykAddr,bool value,bool clearerror)
{
	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	ST_BYTE sendbuf[256] = {0};
	sendbuf[0]	= 0x68;

	sendbuf[1]	= (info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);
	sendbuf[2]	= (info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
	sendbuf[3]	= (info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
	sendbuf[4]	= (info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
	sendbuf[5]	= (info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
	sendbuf[6]	= (info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
	// 命令流水号 低字节(S0)在前，高字节(S1)在后，由主站生成，每次递增1，范围为,0-65535。
	sendbuf[7]	= _counter % 256;
	sendbuf[8]	= _counter / 256;

	sendbuf[9]	= 0x68;
	// C D7  0 主站 1从站 D6从站应答位  0正常应答
	sendbuf[10]	= 0x20;
	// 数据域长度 低前高后吧
	sendbuf[11]	= 0x09;
	sendbuf[12]	= 0x00;
	// 数据域
	sendbuf[13]	= 0x02;
	sendbuf[14]	= 0x02;
	sendbuf[15]	= 0x02;
	sendbuf[16]	= 0x02;
	sendbuf[17]	= 0x02;
	sendbuf[18]	= 0x02;
	sendbuf[19] = 0x02;
	sendbuf[20]	= 0x02;
	sendbuf[12+ykAddr] = value?0x01:0x00;
	sendbuf[21] = clearerror?0x01:0x00;			// 清除错误信息
	sendbuf[22] = CSCheck(sendbuf,22);
	sendbuf[23]	= 0x16;
	this->Send(sendbuf,24);
	_counter++;
}
