#include "SingleAir.h"


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


SingleAir::SingleAir()
{
    //ctor
    m_readidex = 0;
}

SingleAir::~SingleAir()
{
    //dtor
}


SingleAir * CreateInstace()
{
	return new SingleAir();
}


ST_VOID	    SingleAir::OnRead(ST_BYTE* pbuf, ST_INT& readed)
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
                //if (CSCheck(pbuf,ndatalen-2) == pbuf[ndatalen-2])
				//{
					readed = ndatalen;
                    return;
                    /*
				}
				else
                {
                    this->ShowRecvFrame(pbuf,ndatalen);
                    this->ShowMessage("check error!");
                    this->GetCurPort()->Clear();
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

ST_BOOLEAN	SingleAir::OnSend()
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
	switch(m_readidex){
    case 0:
        AskState();
        break;
    case 1:
        AskPQ();
        break;
    default:
        AskState();
        break;
	}
	if(m_readidex++ == 2) m_readidex = 0;
    //AskState();
    return true;
}

ST_BOOLEAN	SingleAir::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    switch(pbuf[10])
	{
	case 0xD1:{ // 普通水泵/单速风机控制器查询应答
				Analysis21H(&pbuf[13]);
			  }break;
	case 0xD0:{	// 遥控返回
				m_curTask.taskResult.resultCode = 0;
                m_curTask.isTransfer = 1;
                Transfer(&m_curTask);
                Memset(&m_curTask, 0, sizeof(m_curTask));
			  }break;
    case 0x86:
        {
            this->UpdateValue(10000,bcd2Fvalue(&pbuf[13],4)); //13 14 15 16
            this->UpdateValue(10001,bcd2Fvalue(&pbuf[17],4)); //17 18 19 20
            this->UpdateValue(10002,bcd2Fvalue(&pbuf[21],4)); //21 22 23 24
            this->UpdateValue(10003,bcd2Fvalue(&pbuf[25],4)); //25 26 27 28
        }
	default:break;
	}
    return true;
}

ST_BOOLEAN	SingleAir::IsSupportEngine(ST_INT engineType)
{
    return 1;
}


void SingleAir::AskState()
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
	sendbuf[10]	= 0x51; //普通水泵/单速风机控制器状态查询
	sendbuf[11]	= 0x00;
	sendbuf[12]	= 0x00;

	sendbuf[13]	= CSCheck(sendbuf,13);
	sendbuf[14]	= 0x16;
	this->Send(sendbuf,15);
	_counter++;

}

void    SingleAir::AskPQ()
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
	sendbuf[10]	= 0x06; //普通水泵/单速风机控制器状态查询
	sendbuf[11]	= 0x00;
	sendbuf[12]	= 0x00;

	sendbuf[13]	= CSCheck(sendbuf,13);
	sendbuf[14]	= 0x16;
	this->Send(sendbuf,15);
	_counter++;
}

void	SingleAir::Analysis21H(ST_BYTE* pbuf)
{
 //   float fvalue=0;
    //this->UpdateValue(0,pbuf[0]);//自动/手动状态
/*
    for(int i =0;i<4;i++)//ID : 1 2 3 4
    {
        ST_BYTE bvalue = (pbuf[1]>>i)&0x01;
        this->UpdateValue(i+1,bvalue);
    }*/
    ST_BYTE bValue = 0x00;
    bValue = pbuf[0]==0x01?0x01:0x00;
    this->UpdateValue(0,bValue);

    bValue = pbuf[1]==0x01?0x01:0x00;
    this->UpdateValue(1,bValue);

    bValue = pbuf[2]==0x01?0x01:0x00;
    this->UpdateValue(2,bValue);

    bValue = pbuf[3]==0x01?0x01:0x00;
    this->UpdateValue(3,bValue);
    //第1通道
    this->UpdateValue(4,bcd2Fvalue(&pbuf[4],2));
    this->UpdateValue(5,bcd2Fvalue(&pbuf[6],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[8],4)); // 8 9 10 11
    //第2通道
    this->UpdateValue(7,bcd2Fvalue(&pbuf[12],2));
    this->UpdateValue(8,bcd2Fvalue(&pbuf[14],2));
    this->UpdateValue(9,bcd2Fvalue(&pbuf[16],4)); // 16 17 18 19
    //第3通道
    this->UpdateValue(10,bcd2Fvalue(&pbuf[20],2));
    this->UpdateValue(11,bcd2Fvalue(&pbuf[22],2));
    this->UpdateValue(12,bcd2Fvalue(&pbuf[24],4)); // 24 25 26 27
    //第4通道
    this->UpdateValue(13,bcd2Fvalue(&pbuf[28],2));
    this->UpdateValue(14,bcd2Fvalue(&pbuf[30],2));
    this->UpdateValue(15,bcd2Fvalue(&pbuf[32],4)); // 32 33 34 35

    /*
    // A相输入电压 B相输入电压 C相输入电压
    this->UpdateValue(4,bcd2Fvalue(&pbuf[4],2));
    this->UpdateValue(5,bcd2Fvalue(&pbuf[6],2));
    this->UpdateValue(6,bcd2Fvalue(&pbuf[8],2));

    this->UpdateValue(7,pbuf[8]);  //第一通道状态
    this->UpdateValue(8,bcd2Fvalue(&pbuf[10],2));
    this->UpdateValue(9,bcd2Fvalue(&pbuf[11],2));
    this->UpdateValue(10,bcd2Fvalue(&pbuf[13],2));
    this->UpdateValue(11,bcd2Fvalue(&pbuf[15],4)); //15 16 17 18 第一通道实时合相有功功率

    this->UpdateValue(12,pbuf[19]);  //第二通道状态
    this->UpdateValue(13,bcd2Fvalue(&pbuf[20],2));
    this->UpdateValue(14,bcd2Fvalue(&pbuf[22],2));
    this->UpdateValue(15,bcd2Fvalue(&pbuf[24],2));
    this->UpdateValue(16,bcd2Fvalue(&pbuf[26],4)); //26 27 28 29 第二通道实时有功功率*/
}

void	SingleAir::Analysis86H(ST_BYTE*pbuf)
{

}

void SingleAir::SendYK(ST_UINT ykAddr,bool value,bool clearerror)
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
	sendbuf[10]	= 0x50;
	// 数据域长度 低前高后吧
	sendbuf[11]	= 0x05;
	sendbuf[12]	= 0x00;
	// 数据域
	sendbuf[13]	= 0x02;
	sendbuf[14]	= 0x02;
	sendbuf[15]	= 0x02;
	sendbuf[16]	= 0x02;
	sendbuf[12+ykAddr] = value?0x01:0x00;
	sendbuf[17] = clearerror?0x01:0x00;			// 清除错误信息
	sendbuf[18] = CSCheck(sendbuf,22);
	sendbuf[19]	= 0x16;
	this->Send(sendbuf,20);
	_counter++;
}

float  SingleAir::bcd2Fvalue(ST_BYTE* pbuf,int len)
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
