#include "CKWZD.h"
#include "Device.h"
#include "Channel.h"
#include "ChannelConfig.h"
#include "EngineBase.h"
#include "Devices.h"      //通迅设备管理类
CKWZD::CKWZD()
{
    //ctor
}

CKWZD::~CKWZD()
{
    //dtor
}

CKWZD* CreateInstace()
{
    return  new CKWZD();
}

static int  hexcharToInt(char c)
{
	if(c>='A' && c<='Z') return (c-'A')+10;
	if(c>='a' && c<='b') return (c-'a')+10;
	if(c>='0' && c<='9') return (c-'0');
	return 0;
}

static ST_BYTE char2Hex(char ch,char cl)
{
	ST_BYTE buf  = 0x00;
	buf |= (hexcharToInt(ch)<<4);
	buf |= hexcharToInt(cl);
	return buf;
}

static ST_BYTE XORCheck(ST_BYTE * pbuf, int size)
{
	ST_BYTE res = 0x00;
	for (int i = 0;i<size;i++)
	{
		res ^= pbuf[i];
	}
	return res;
}

void	CKWZD::Init()
{
    m_readindex=0;
}
void	CKWZD::Uninit()
{

}
void	CKWZD::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
    readed = 0;

    if(this->GetCurPort())
    {
        int lineInterval  = 10000;
        int	len = this->GetCurPort()->PickBytes(pbuf, 15, lineInterval);
        if (len >= 15)
        {
            int star = 0;
            for(; star < len; ++star)
            {
                if(pbuf[star] == 0x7E)
                    break;
            }
            if(star == len)
            {
                this->ShowMessage("Is Messy Code!");
                this->ShowRecvFrame(pbuf,len);
                this->GetCurPort()->Clear();
                return;
            }
            if (star > 0)
            {
                this->GetCurPort()->ReadBytes(pbuf, star);
            }
            len = this->GetCurPort()->PickBytes(pbuf, 15, lineInterval);
            if (len < 15)
            {
                this->ShowMessage("Insufficient data length!");
                this->ShowRecvFrame(pbuf, len);
                this->GetCurPort()->Clear();
                return;
            }
            len =  pbuf[13]*256+ pbuf[14]+ 17;
            int nlen = this->GetCurPort()->PickBytes(pbuf, len, lineInterval);

            if(nlen == len)
            {
                /*
                if (memcmp(pbuf + 6, m_addrarea, sizeof(m_addrarea)))
                {
                	this->ShowMessage("address", 0);
                	this->ShowReceiveFrame(pbuf,len);
                	return;
                }*/
                readed  = len;
                return;

            }
            else
            {
                this->ShowMessage("Insufficient data length!");
                this->ShowRecvFrame(pbuf,len);
                this->GetCurPort()->Clear();
            }
        }
        else
        {
            this->ShowMessage("Insufficient data length!");
            this->ShowRecvFrame(pbuf,len);
            this->GetCurPort()->Clear();
        }
    }

}
ST_BOOLEAN	CKWZD::OnSend()
{
    if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 1)
				SendYK(m_curTask.taskValue);
		}
		return true;
	}
    switch(m_readindex)
    {
    case 0:
        Read_Realtime_Data();   //读取实时值
        break;
    case 1:
        Read_Device_Data();     //读取设备信息
        break;
    /*	case 2:
    		Read_EMeter_Data();		//读取电表参数
    		break;*/
    case 2:
        Read_History_Data();	//读取历史数据
        break;
    default:
        break;
    }
    if((++m_readindex)==3)m_readindex = 0;

    return true;
}
ST_BOOLEAN	CKWZD::OnProcess(ST_BYTE * pbuf, ST_INT len)
{
    int counts = this->m_pChannel->GetDevices()->GetCount();
    for(int i=0;i<counts;i++){
        this->m_pChannel->GetDevices()->GetItem(i);

    }

    List<ProtocolBase *>	t_Protocols ;
    t_Protocols = m_pChannel->GetCEngine()->m_pProtocols;

    for(int j = 0; j < t_Protocols.GetCount(); j++)
    {
        CKWZD  *tp  = (CKWZD *)(*t_Protocols.GetItem(j));
        if(tp != NULL)
        {
            if((tp->GetDevice()->GetDeviceInfo()->Addressex)==NULL)
                continue;
/*
            if(strncmp((const char*)&pbuf[1],(const char*)tp->GetDevice()->GetDeviceInfo()->Addressex,9)==0)
            {
                //ShowMessage("get true protocols");
                tp->processBuf(pbuf);
                return 1;
            }
            */
            if (!memcmp(pbuf + 6, tp->m_addrarea, sizeof(tp->m_addrarea)))
            {
                tp->processBuf(pbuf,len);
                /*this->OnShowMsg("地址域不匹配", 0);
                this->ShowReceiveFrame(pbuf,len);*/
                return true;
            }
        }

    }

    return true;
}


ST_BOOLEAN	CKWZD::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

bool	CKWZD::processBuf(ST_BYTE* pbuf,int len)
{
	if (pbuf[3] == 0x03)
	{
		switch(pbuf[4]){
		case 0x02: //遥控返回
		case 0x03:
			{
				if (pbuf[15] == 0xAA)  //=0xaa 接受
				{
					//0代表成功
					m_curTask.taskResult.resultCode = 0;
                    m_curTask.isTransfer = 1;
                    Transfer(&m_curTask);
                    Memset(&m_curTask, 0, sizeof(m_curTask));
                    return true;
				}
				else if (pbuf[15] == 0x55)
				{
					//1代表失败
					m_curTask.taskResult.resultCode = -1;
                    m_curTask.isTransfer = 1;
                    Transfer(&m_curTask);
                    Memset(&m_curTask, 0, sizeof(m_curTask));
                    return true;
				}
			}break;
		case 0x05: //实时数据返回
			{
				if(len<50){
				this->ShowMessage("message is too short");
					return false;				 //屏蔽掉乱码
				}

				Analy_realtime_Data(&pbuf[19],(pbuf[14]>0x90)?true:false); //直接解析数据域
			}break;
		case 0x06:
			{
				if(len<50){
				this->ShowMessage("message is too short");
					return false;				 //屏蔽掉乱码
				}
				Analy_History_Data(&pbuf[23]);
			}
		default:
			break;
		}
	}
	else if (pbuf[3] == 0x01)
	{
		if(pbuf[4] == 0x02)
		{
			if(len<50){
				this->ShowMessage("message is too short");
				return false;				 //屏蔽掉乱码
			}
			Analy_Device_Data(&pbuf[15]);//设备信息解析//设备信息解析
		}
		else if (pbuf[4] == 0x04)
		{
			//Analy_EMeter_Data(&pbuf[15]);
		}

	}
	return true;
}


void    CKWZD::Read_Realtime_Data()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x7E;
	sendbuf[1] = 0x10;
	sendbuf[2] = 0x10;
	sendbuf[3] = 0x03;
	sendbuf[4] = 0x05;

	if (info.Addressex[0])
	{
		sendbuf[11] = char2Hex(info.Addressex[0],info.Addressex[1]);//(info.Addressex[12]-0x30)*16 + (info.Addressex[13]-0x30);
		sendbuf[10] = char2Hex(info.Addressex[2],info.Addressex[3]);//(info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
		sendbuf[9] = char2Hex(info.Addressex[4],info.Addressex[5]);//(info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
		sendbuf[8] = char2Hex(info.Addressex[6],info.Addressex[7]);//(info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
		sendbuf[7] = char2Hex(info.Addressex[8],info.Addressex[9]);//(info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
		sendbuf[6] = char2Hex(info.Addressex[10],info.Addressex[11]);//(info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
		sendbuf[5] = char2Hex(info.Addressex[12],info.Addressex[13]);//(info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);

		sendbuf[12] = char2Hex(info.Addressex[14],info.Addressex[15]);//0x00; //RS485
		memcpy(m_addrarea,&sendbuf[5],sizeof(m_addrarea));
	}
	else
	{
		sendbuf[5] = 0xFF;
		sendbuf[6] = 0xFF;
		sendbuf[7] = 0xFF;
		sendbuf[8] = 0xFF;
		sendbuf[9] = 0xFF;
		sendbuf[10] = 0xFF;
		sendbuf[11] = 0xFF;
		sendbuf[12] = 0xFF;
	}


	sendbuf[13] = 0x00;
	sendbuf[14] = 0x04;  //len = 4

	time_t t;
	time(&t);
	ST_INT32 time32 = (ST_INT32)t;


	sendbuf[18] = time32;
	sendbuf[17] = time32>>8;
	sendbuf[16] = time32>>16;
	sendbuf[15] = time32>>24;

	//*(WORD*)(sendbuf + 19) = GetCRC16(sendbuf,19);
	sendbuf[19] = 0x00;
	sendbuf[20] = XORCheck(&sendbuf[1],18);
	sendbuf[21] = 0x0D;
	this->Send(sendbuf,22);
}

void	CKWZD::Read_History_Data()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x7E;
	sendbuf[1] = 0x10;
	sendbuf[2] = 0x10;
	sendbuf[3] = 0x03;
	sendbuf[4] = 0x06;

	if (info.Addressex[0])
	{
		sendbuf[11] = char2Hex(info.Addressex[0],info.Addressex[1]);//(info.Addressex[12]-0x30)*16 + (info.Addressex[13]-0x30);
		sendbuf[10] = char2Hex(info.Addressex[2],info.Addressex[3]);//(info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
		sendbuf[9] = char2Hex(info.Addressex[4],info.Addressex[5]);//(info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
		sendbuf[8] = char2Hex(info.Addressex[6],info.Addressex[7]);//(info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
		sendbuf[7] = char2Hex(info.Addressex[8],info.Addressex[9]);//(info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
		sendbuf[6] = char2Hex(info.Addressex[10],info.Addressex[11]);//(info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
		sendbuf[5] = char2Hex(info.Addressex[12],info.Addressex[13]);//(info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);

		sendbuf[12] = char2Hex(info.Addressex[14],info.Addressex[15]);//0x00; //RS485

		memcpy(m_addrarea,&sendbuf[5],sizeof(m_addrarea));
	}
	else
	{
		sendbuf[5] = 0xFF;
		sendbuf[6] = 0xFF;
		sendbuf[7] = 0xFF;
		sendbuf[8] = 0xFF;
		sendbuf[9] = 0xFF;
		sendbuf[10] = 0xFF;
		sendbuf[11] = 0xFF;
		sendbuf[12] = 0xFF;
	}


	sendbuf[13] = 0x00;
	sendbuf[14] = 0x04;  //len = 4

	sendbuf[15] = 0x00;
	sendbuf[16] = 0x00;
	sendbuf[17] = 0x00;
	sendbuf[18] = 0x02;

	//*(WORD*)(sendbuf + 19) = GetCRC16(sendbuf,19);
	sendbuf[19] = 0x00;
	sendbuf[20] = XORCheck(&sendbuf[1],18);
	sendbuf[21] = 0x0D;
	this->Send(sendbuf,22);
}

void	CKWZD::Read_EMeter_Data()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
		ST_BYTE sendbuf[256];
	sendbuf[0] = 0x7E;

	sendbuf[1] = 0x10;
	sendbuf[2] = 0x10;
	sendbuf[3] = 0x01;
	sendbuf[4] = 0x04;  //读取电表参数


	if (info.Addressex[0])
	{
		sendbuf[11] = char2Hex(info.Addressex[0],info.Addressex[1]);//(info.Addressex[12]-0x30)*16 + (info.Addressex[13]-0x30);
		sendbuf[10] = char2Hex(info.Addressex[2],info.Addressex[3]);//(info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
		sendbuf[9] = char2Hex(info.Addressex[4],info.Addressex[5]);//(info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
		sendbuf[8] = char2Hex(info.Addressex[6],info.Addressex[7]);//(info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
		sendbuf[7] = char2Hex(info.Addressex[8],info.Addressex[9]);//(info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
		sendbuf[6] = char2Hex(info.Addressex[10],info.Addressex[11]);//(info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
		sendbuf[5] = char2Hex(info.Addressex[12],info.Addressex[13]);//(info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);

		sendbuf[12] = char2Hex(info.Addressex[14],info.Addressex[15]);//0x00; //RS485
		memcpy(m_addrarea,&sendbuf[5],sizeof(m_addrarea));
	}
	else
	{
		sendbuf[5] = 0xFF;
		sendbuf[6] = 0xFF;
		sendbuf[7] = 0xFF;
		sendbuf[8] = 0xFF;
		sendbuf[9] = 0xFF;
		sendbuf[10] = 0xFF;
		sendbuf[11] = 0xFF;
		sendbuf[12] = 0xFF;
	}

	sendbuf[13] = 0x00;
	sendbuf[14] = 0x00;  //len = 0

	//*(WORD*)(sendbuf + 14) = GetCRC16(sendbuf,14);
	sendbuf[15] = 0x00;
	sendbuf[16] = XORCheck(&sendbuf[1],14);

	sendbuf[17] = 0x0D;
	this->Send(sendbuf,18);
}

void	CKWZD::Read_Device_Data()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x7E;

	sendbuf[1] = 0x10;
	sendbuf[2] = 0x10;
	sendbuf[3] = 0x01;
	sendbuf[4] = 0x02;

	if (info.Addressex[0])
	{
		sendbuf[11] = char2Hex(info.Addressex[0],info.Addressex[1]);//(info.Addressex[12]-0x30)*16 + (info.Addressex[13]-0x30);
		sendbuf[10] = char2Hex(info.Addressex[2],info.Addressex[3]);//(info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
		sendbuf[9] = char2Hex(info.Addressex[4],info.Addressex[5]);//(info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
		sendbuf[8] = char2Hex(info.Addressex[6],info.Addressex[7]);//(info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
		sendbuf[7] = char2Hex(info.Addressex[8],info.Addressex[9]);//(info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
		sendbuf[6] = char2Hex(info.Addressex[10],info.Addressex[11]);//(info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
		sendbuf[5] = char2Hex(info.Addressex[12],info.Addressex[13]);//(info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);

		sendbuf[12] = char2Hex(info.Addressex[14],info.Addressex[15]);//0x00; //RS485
		memcpy(m_addrarea,&sendbuf[5],sizeof(m_addrarea));
	}
	else
	{
		sendbuf[5] = 0xFF;
		sendbuf[6] = 0xFF;
		sendbuf[7] = 0xFF;
		sendbuf[8] = 0xFF;
		sendbuf[9] = 0xFF;
		sendbuf[10] = 0xFF;
		sendbuf[11] = 0xFF;
		sendbuf[12] = 0xFF;
	}

	sendbuf[13] = 0x00;
	sendbuf[14] = 0x00;  //len = 0

	//*(WORD*)(sendbuf + 14) = GetCRC16(sendbuf,14);
	sendbuf[15] = 0x00;
	sendbuf[16] = XORCheck(&sendbuf[1],14);

	sendbuf[17] = 0x0D;
	this->Send(sendbuf,18);
}

void    CKWZD::SendYK(int bValue)
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
	ST_BYTE sendbuf[256];
	sendbuf[0] = 0x7E;

	sendbuf[1] = 0x10;
	sendbuf[2] = 0x10;
	sendbuf[3] = 0x03;
	sendbuf[4] = bValue == 1? 0x02:0x03 ;

	if (info.Addressex[0])
	{
		sendbuf[11] = char2Hex(info.Addressex[0],info.Addressex[1]);//(info.Addressex[12]-0x30)*16 + (info.Addressex[13]-0x30);
		sendbuf[10] = char2Hex(info.Addressex[2],info.Addressex[3]);//(info.Addressex[10]-0x30)*16 + (info.Addressex[11]-0x30);
		sendbuf[9] = char2Hex(info.Addressex[4],info.Addressex[5]);//(info.Addressex[8]-0x30)*16 + (info.Addressex[9]-0x30);
		sendbuf[8] = char2Hex(info.Addressex[6],info.Addressex[7]);//(info.Addressex[6]-0x30)*16 + (info.Addressex[7]-0x30);
		sendbuf[7] = char2Hex(info.Addressex[8],info.Addressex[9]);//(info.Addressex[4]-0x30)*16 + (info.Addressex[5]-0x30);
		sendbuf[6] = char2Hex(info.Addressex[10],info.Addressex[11]);//(info.Addressex[2]-0x30)*16 + (info.Addressex[3]-0x30);
		sendbuf[5] = char2Hex(info.Addressex[12],info.Addressex[13]);//(info.Addressex[0]-0x30)*16 + (info.Addressex[1]-0x30);

		sendbuf[12] = char2Hex(info.Addressex[14],info.Addressex[15]);//0x00; //RS485
		memcpy(m_addrarea,&sendbuf[5],sizeof(m_addrarea));
	}
	else
	{
		sendbuf[5] = 0xFF;
		sendbuf[6] = 0xFF;
		sendbuf[7] = 0xFF;
		sendbuf[8] = 0xFF;
		sendbuf[9] = 0xFF;
		sendbuf[10] = 0xFF;
		sendbuf[11] = 0xFF;
		sendbuf[12] = 0xFF;
	}


	sendbuf[13] = 0x00;
	sendbuf[14] = 0x00;  //len = 0

	//*(WORD*)(sendbuf + 14) = GetCRC16(sendbuf,14);
	sendbuf[15] = 0x00;
	sendbuf[16] = XORCheck(&sendbuf[1],14);

	sendbuf[17] = 0x0D;
	this->Send(sendbuf,18);
}

float	CKWZD::calculate_value(ST_BYTE *pbuf,float coeff)
{
	ST_INT32 fvalue = 0;
	fvalue = fvalue | pbuf[3];
	fvalue = fvalue | ((ST_INT32)pbuf[2]<<8);
	fvalue = fvalue | ((ST_INT32)pbuf[1]<<16);
	fvalue = fvalue | ((ST_INT32)pbuf[0]<<24);
	float fv = ((float)fvalue * coeff);
	return fv;
}



void  CKWZD::Analy_Device_Data(ST_BYTE *pbuf)
{
	for (int i= 0;i <26;i++)
	{
		ST_INT32 fvalue = 0;
		fvalue = fvalue | pbuf[3+i*4];
		fvalue = fvalue | ((ST_INT32)pbuf[2+i*4]<<8);
		fvalue = fvalue | ((ST_INT32)pbuf[1+i*4]<<16);
		fvalue = fvalue | ((ST_INT32)pbuf[0+i*4]<<24);
		float fv = (float)fvalue;


//		switch(i){
//		case 28:
//		case 29:
//		case 30:
//		case 31:
//		case 32:
//		case 33:
//		case 34:
// 		case 19:
// 		case 20:
// 		case 37://温度数据需要减去50
// 			{
// 				fv = fv - 50;
// 			}break;
// 		default:
// 			break;
// 		}

		this->UpdateValue(i+10000,(float)fv);
	}
}

void    CKWZD::Analy_EMeter_Data(ST_BYTE *pbuf)
{
	for (int i= 0;i <9;i++)
	{
		ST_UINT32 fvalue = 0;
		fvalue = fvalue | pbuf[3+i*4];
		fvalue = fvalue | ((ST_UINT32)pbuf[2+i*4]<<8);
		fvalue = fvalue | ((ST_UINT32)pbuf[1+i*4]<<16);
		fvalue = fvalue | ((ST_UINT32)pbuf[0+i*4]<<24);
		float fv = (float)fvalue;


		this->UpdateValue(i+30000,(float)fv);
	}
}

void  CKWZD::Analy_realtime_Data(ST_BYTE *pbuf,bool is_new)
{
	//A B C相电压
	{
		float fv = calculate_value(pbuf,0.1);
		this->UpdateValue(0,fv);

		fv = calculate_value(&pbuf[4],0.1);
		this->UpdateValue(1,fv);

		fv = calculate_value(&pbuf[8],0.1);
		this->UpdateValue(2,fv);
	}

	//A B C相电流
	{
		float fv = calculate_value(&pbuf[12],0.01);
		this->UpdateValue(3,fv);

		fv = calculate_value(&pbuf[16],0.01);
		this->UpdateValue(4,fv);

		fv = calculate_value(&pbuf[20],0.01);
		this->UpdateValue(5,fv);
	}

	//漏电电流
	{
		float fv = calculate_value(&pbuf[24],0.1);
		this->UpdateValue(6,fv);
	}

	//A B C相功率
	{
		float fv = calculate_value(&pbuf[28],0.0001);
		this->UpdateValue(7,fv);

		fv = calculate_value(&pbuf[32],0.0001);
		this->UpdateValue(8,fv);

		fv = calculate_value(&pbuf[36],0.0001);
		this->UpdateValue(9,fv);
	}
	//A B C相电量
	{
		float fv = calculate_value(&pbuf[40],0.001);
		this->UpdateValue(10,fv);

		fv = calculate_value(&pbuf[44],0.001);
		this->UpdateValue(11,fv);

		fv = calculate_value(&pbuf[48],0.001);
		this->UpdateValue(12,fv);
	}
	//A B C N相开关温度  HWD 环境温度  A 相触头温升
	{
		float fv = calculate_value(&pbuf[52],0.1);
		this->UpdateValue(13,fv-50);

		fv = calculate_value(&pbuf[56],0.1);
		this->UpdateValue(14,fv-50);

		fv = calculate_value(&pbuf[60],0.1);
		this->UpdateValue(15,fv-50);

		fv = calculate_value(&pbuf[64],0.1);
		this->UpdateValue(16,fv-50);

		fv = calculate_value(&pbuf[68],0.1);  //主控芯片温度
		this->UpdateValue(17,fv-50);

		fv = calculate_value(&pbuf[72],0.1);  //HWD 环境温度
		this->UpdateValue(18,fv-50);

		fv = calculate_value(&pbuf[76],0.1);
		this->UpdateValue(19,fv);

		fv = calculate_value(&pbuf[80],0.1);
		this->UpdateValue(20,fv);

		fv = calculate_value(&pbuf[84],0.1);
		this->UpdateValue(21,fv);

		fv = calculate_value(&pbuf[88],0.1);
		this->UpdateValue(22,fv); //88 89 90 91
	}


	//信号类型//92 93 94 95  //88 89 90 91
	{
		ST_BYTE bvalue;
		bvalue = ((pbuf[95]>>1)&0x01);
		this->UpdateValue(23,bvalue);

		bvalue = ((pbuf[95]>>2)&0x01);
		this->UpdateValue(24,bvalue);

		bvalue = ((pbuf[95]>>3)&0x01);
		this->UpdateValue(25,bvalue);

		bvalue = ((pbuf[95]>>4)&0x01);
		this->UpdateValue(26,bvalue);

		bvalue = ((pbuf[95]>>5)&0x01);
		this->UpdateValue(27,bvalue);

		bvalue = ((pbuf[95]>>6)&0x01);
		this->UpdateValue(28,bvalue);
// 	}
// 	{

		bvalue = ((pbuf[94])&0x01);
		this->UpdateValue(29,bvalue);

		bvalue = ((pbuf[94]>>1)&0x01);
		this->UpdateValue(30,bvalue);

		bvalue = ((pbuf[94]>>2)&0x01);
		this->UpdateValue(31,bvalue);

		bvalue = ((pbuf[94]>>3)&0x01);
		this->UpdateValue(32,bvalue);

		bvalue = ((pbuf[94]>>4)&0x01);
		this->UpdateValue(33,bvalue);

		bvalue = ((pbuf[94]>>5)&0x01);
		this->UpdateValue(34,bvalue);

		bvalue = ((pbuf[94]>>6)&0x01);
		this->UpdateValue(35,bvalue);

		bvalue = ((pbuf[94]>>7)&0x01);
		this->UpdateValue(36,bvalue);
// 	}
//
// 	{
		bvalue = ((pbuf[93])&0x01);
		this->UpdateValue(37,bvalue);

		bvalue = ((pbuf[93]>>1)&0x01);
		this->UpdateValue(38,bvalue);

		bvalue = ((pbuf[93]>>2)&0x01);
		this->UpdateValue(39,bvalue);

		bvalue = ((pbuf[93]>>3)&0x01);
		this->UpdateValue(40,bvalue);

		bvalue = ((pbuf[93]>>5)&0x01);
		this->UpdateValue(41,bvalue);

		bvalue = ((pbuf[93]>>6)&0x01);
		this->UpdateValue(42,bvalue);

		bvalue = ((pbuf[93]>>7)&0x01);
		this->UpdateValue(43,bvalue);
// 	}
// 	{
		bvalue = ((pbuf[92])&0x01);
		this->UpdateValue(44,bvalue);

		bvalue = ((pbuf[92]>>1)&0x01);
		this->UpdateValue(45,bvalue);

		bvalue = ((pbuf[92]>>2)&0x01);
		this->UpdateValue(46,bvalue);

		bvalue = ((pbuf[92]>>3)&0x01);
		this->UpdateValue(47,bvalue);

		bvalue = ((pbuf[92]>>4)&0x01);
		this->UpdateValue(48,bvalue);

		bvalue = ((pbuf[92]>>5)&0x01);
		this->UpdateValue(49,bvalue);

		bvalue = ((pbuf[92]>>6)&0x01);
		this->UpdateValue(50,bvalue);

		bvalue = ((pbuf[92]>>7)&0x01);
		this->UpdateValue(51,bvalue);
	}
		//二期开发
	{
		float fv = calculate_value(&pbuf[96],1);
		this->UpdateValue(52,fv);

		fv = calculate_value(&pbuf[100],1);
		this->UpdateValue(53,fv);

		fv = calculate_value(&pbuf[104],1);
		this->UpdateValue(54,fv);

		fv = calculate_value(&pbuf[108],1);
		this->UpdateValue(55,fv);

		fv = calculate_value(&pbuf[112],1);
		this->UpdateValue(56,fv);

		fv = calculate_value(&pbuf[116],1);
		this->UpdateValue(57,fv);

		fv = calculate_value(&pbuf[120],1);
		this->UpdateValue(58,fv);

		fv = calculate_value(&pbuf[124],1);
		this->UpdateValue(59,fv);

		fv = calculate_value(&pbuf[128],1);
		this->UpdateValue(60,fv);
	}

	//新增6个
	{
		/*
		功率因素单位 96
		正弦电频率
		A相角度
		B相角度
		C相角度
		三相不平衡度
		*/
		//OnShowMsg("Enter function",0);
		if(is_new){

			//OnShowMsg("analyze new data",0);
			ST_BYTE bvalue = ((pbuf[135])&0x01);
			this->UpdateValue(61,bvalue);

			bvalue = ((pbuf[135]>>1)&0x01);
			this->UpdateValue(62,bvalue);

			bvalue = ((pbuf[135]>>2)&0x01);
			this->UpdateValue(63,bvalue);

			bvalue = ((pbuf[135]>>3)&0x01);
			this->UpdateValue(64,bvalue);

			float fv = calculate_value(&pbuf[136],1);
			this->UpdateValue(65,fv);

			fv = calculate_value(&pbuf[140],1);
			this->UpdateValue(66,fv);

			fv = calculate_value(&pbuf[144],1);
			this->UpdateValue(67,fv);

			fv = calculate_value(&pbuf[148],1);
			this->UpdateValue(68,fv);

			fv = calculate_value(&pbuf[152],1);
			this->UpdateValue(69,fv);
/*
			fv = calculate_value(&pbuf[156],1);
			this->UpdateValue(70,fv,&pbuf[156],4);
			char msg[64];
			sprintf(msg,"三相不平衡度：%f",fv);
			OnShowMsg(msg,0);*/
		}

	}
}

void    CKWZD::Analy_History_Data(ST_BYTE *pbuf)
{
	int itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[4].itemCount;
	for (int itemindex = 0; itemindex < itemsize; ++itemindex)
	{
		const DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[4].items[itemindex];
		float coef = itemref.coeficient;
		if(coef == 0)
			coef = 1;
		float fv = calculate_value(&pbuf[0+4*itemindex],coef);
		this->UpdateValue(itemref.id,fv);
	}

}
