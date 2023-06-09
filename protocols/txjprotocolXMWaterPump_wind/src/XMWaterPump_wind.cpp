#include "XMWaterPump_wind.h"
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
XMWaterPump_wind::XMWaterPump_wind()
{
    //ctor
    _counter = 0;
}

XMWaterPump_wind::~XMWaterPump_wind()
{
    //dtor
}

XMWaterPump_wind * CreateInstace()
{
	return new XMWaterPump_wind();
}


ST_VOID	    XMWaterPump_wind::OnRead(ST_BYTE* pbuf, ST_INT& readed)
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
                if (CSCheck(pbuf,ndatalen-2) == pbuf[ndatalen-2])
				{
					readed = ndatalen;
                    return;
				}
				else
                {
                    this->ShowMessage("check error!");
					return;
                }


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

ST_BOOLEAN	XMWaterPump_wind::OnSend()
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

ST_BOOLEAN	XMWaterPump_wind::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    switch(pbuf[10])
	{
	case 0xB1:{ // 普通水泵/单速风机控制器查询应答
				Analysis21H(&pbuf[13]);
			  }break;
	case 0xB0:{	// 遥控返回
				m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                Memset(&m_curTask, 0, sizeof(m_curTask));
			  }break;
	default:break;
	}
    return true;
    return 1;
}

ST_BOOLEAN	XMWaterPump_wind::IsSupportEngine(ST_INT engineType)
{
    return 1;
}


void XMWaterPump_wind::AskState()
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
	sendbuf[10]	= 0x31; //普通水泵/单速风机控制器状态查询
	sendbuf[11]	= 0x00;
	sendbuf[12]	= 0x00;

	sendbuf[13]	= CSCheck(sendbuf,13);
	sendbuf[14]	= 0x16;
	this->Send(sendbuf,15);
	_counter++;

}

void	XMWaterPump_wind::Analysis21H(ST_BYTE* pbuf)
{
 //   float fvalue=0;
/*     this->UpdateValue(0,bcd2Fvalue(&pbuf[0],2));

    this->UpdateValue(1,bcd2Fvalue(&pbuf[2],2));
    this->UpdateValue(2,bcd2Fvalue(&pbuf[4],2));

    this->UpdateValue(3,pbuf[6]);//第一通道状态
    this->UpdateValue(4,bcd2Fvalue(&pbuf[7],2));
    this->UpdateValue(5,bcd2Fvalue(&pbuf[9],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[11],2));
    this->UpdateValue(7,bcd2Fvalue(&pbuf[13],4)); //13 14 15 16 第一通道实时合相有功功率

    this->UpdateValue(8,pbuf[17]);//第二通道状态
    this->UpdateValue(4,bcd2Fvalue(&pbuf[18],2));
    this->UpdateValue(5,bcd2Fvalue(&pbuf[20],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[22],2));
    this->UpdateValue(7,bcd2Fvalue(&pbuf[24],4)); //24 25 26 27 第二通道实时有功功率

    this->UpdateValue(8,pbuf[28]);//第三通道状态
    this->UpdateValue(4,bcd2Fvalue(&pbuf[29],2));
    this->UpdateValue(5,bcd2Fvalue(&pbuf[31],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[33],2));
    this->UpdateValue(7,bcd2Fvalue(&pbuf[35],4)); //35 36 37 38 第三通道实时有功功率
*/
    this->UpdateValue(0,pbuf[0]);//自动/手动状态

    for(int i =0;i<4;i++)//ID : 1 2 3 4
    {
        ST_BYTE bvalue = (pbuf[1]>>i)&0x01;
        this->UpdateValue(i+1,bvalue);
    }
    // A相输入电压 B相输入电压 C相输入电压
    this->UpdateValue(5,bcd2Fvalue(&pbuf[2],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[4],2));
    this->UpdateValue(7,bcd2Fvalue(&pbuf[6],2));

    this->UpdateValue(8,pbuf[8]);  //第一通道状态
    this->UpdateValue(9,bcd2Fvalue(&pbuf[9],2));
    this->UpdateValue(10,bcd2Fvalue(&pbuf[11],2));
    this->UpdateValue(11,bcd2Fvalue(&pbuf[13],2));
    this->UpdateValue(12,bcd2Fvalue(&pbuf[15],4)); // 15 16 17 18第一通道实时合相有功功率

}


void XMWaterPump_wind::SendYK(ST_UINT ykAddr,bool value,bool clearerror)
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
	sendbuf[10]	= 0x30;
	// 数据域长度 低前高后吧
	sendbuf[11]	= 0x03;
	sendbuf[12]	= 0x00;
	// 数据域
	sendbuf[13]	= value?0x01:0x00;
	sendbuf[14]	= 0x00;
//	sendbuf[12+ykAddr] = value?0x01:0x00;
	sendbuf[15] = clearerror?0x01:0x00;			// 清除错误信息
	sendbuf[16] = CSCheck(sendbuf,16);
	sendbuf[17]	= 0x16;
	this->Send(sendbuf,18);
	_counter++;
}

float  XMWaterPump_wind::bcd2Fvalue(ST_BYTE* pbuf,int len)
{
    float value=0;
    if(len == 2)
    {
        value = BcdToDec(pbuf[0]) + BcdToDec(pbuf[1])*100;
        value = value * 0.1;
    }
    else if(len == 4)
    {
        value = BcdToDec(pbuf[0]) + BcdToDec(pbuf[1])*100 + BcdToDec(pbuf[2])*10000 + BcdToDec(pbuf[3])*1000000;
        value = value * 0.1;
    }
    return value;
}
