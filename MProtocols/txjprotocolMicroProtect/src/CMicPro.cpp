#include "CMicPro.h"
#include "Channel.h"
#include "datetime.h"
#include "ChannelConfig.h"
CMicPro::CMicPro()
{
    //ctor
    m_sendqueue.push_back(QUERYDATA);
    m_isYK = false;
}

CMicPro::~CMicPro()
{
    //dtor
}

CMicPro* CreateInstace()
{
    return new CMicPro();
}

void	CMicPro::Init()
{

}
void	CMicPro::Uninit()
{

}
void	CMicPro::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed = 0;
	if(! this->GetCurPort())
		return;

    ST_INT  interval = 1000;//(interval > 2000 ? interval : 2000);

	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 9, interval);
	if(len < 9)
    {
        ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
    }
    if((pbuf[0] == 0xEB)&&(pbuf[1] == 0x90)&&(pbuf[2] == 0xEB)&&(pbuf[3] == 0x90))
    {
        len = pbuf[7] + pbuf[8] * 256 + 9;

        ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 1000);
        if(nlen == len)
        {
            if((pbuf[0] == 0xEB)&&(pbuf[1] == 0x90)&&(pbuf[2] == 0xEB)&&(pbuf[3] == 0x90))
            {
                this->GetCurPort()->ReadBytes(pbuf,len,1000);
                readed = len;
                return ;
            }
            else
            {
                ShowMessage ("data struct error!");
                this->GetCurPort()->Clear();
                return ;
            }
        }
        else
        {
            ShowMessage ("Insufficient data length.");
            this->GetCurPort()->Clear();
        }

    }
    else
    {
        //全是乱码
		ShowMessage ("Garbled code, clear buffer.");
		this->ShowRecvFrame(pbuf,len);
		this->GetCurPort()->Clear();
		return;
    }



}
ST_BOOLEAN	CMicPro::OnSend()
{
    if(m_sendqueue.empty())
        m_sendqueue.push_back(QUERYDATA);

    SENDTYPE step = m_sendqueue.front();

    if (this->HasTask() && this->GetTask(&m_curTask))
	{

		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(m_curTask.taskCmdCode == 1) {
                m_sendqueue.push_front(YKSELECT);
				//return true;
			}

		}
	}


    switch(step){

    case QUERYDATA:
        SendEasyOrder(QUERYDATA);
        break;
    case C12SOE:
        SendEasyOrder(C12SOE);
        break;
    case YKSELECT:
        SendYkSelect((bool)m_curTask.taskValue);
        break;
    case YKEXECUT:
        SendYkExecut((bool)m_curTask.taskValue);
    default:
        SendEasyOrder(QUERYDATA);
        break;
    }

    m_sendqueue.pop_front();
    return true;
}
ST_BOOLEAN	CMicPro::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    ST_BYTE msgtype = pbuf[6];

    switch(msgtype){
    case TypeCode::RACK:{
        ShowMessage("message receive the correct!");
    }

    case TypeCode::SOE:{
        ShowMessage("parse soe data!");
        //this->m_pLogger->LogInfo("receive SOE info!");
        AnalyzerSOE(&pbuf[10]);
        m_sendqueue.push_front(SENDTYPE::C12SOE);
    }break;

    case TypeCode::EVENT:{
        ShowMessage("parse event data!");
        AnalyzerEvent(&pbuf[10]);
        //this->m_pLogger->LogInfo("receive event info!");
        m_sendqueue.push_front(SENDTYPE::C12SOE);
    }break;

    case TypeCode::YCYXDATA:{
        ShowMessage("parse ycyx data!");
        AnalyzerYCYX(&pbuf[7]);
    }break;

    case TypeCode::EQDATA:{
        AnalyzerEq(&pbuf[10]);
    }break;

    case TypeCode::YKSELRETURN:{
        this->m_pLogger->LogInfo("receive YKSELRETURN info!");
        m_sendqueue.push_front(YKEXECUT);
        m_isYK = true;
    }break;

    case TypeCode::YKEXESUCCESS:{
        if(m_isYK){
            ShowMessage("YK successful");
            this->m_pLogger->LogInfo("receive YK successful info!");
            m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
            memset(&m_curTask, 0, sizeof(m_curTask));

            m_isYK = false;
        }

    }break;

    default :
        ShowMessage("not found this type!");
        break;
    }


    return true;
}

ST_BOOLEAN	CMicPro::IsSupportEngine(ST_INT engineType)
{
    return 1;
}


/*
EBH	报
文
头
90H
EBH
90H
02H	起始码（STX）
01H	地址
48H	特征码
04H	报文长度低字节
00H	报文长度高字节
01H	报文内容(本报文中为地址)
4DH	代码和低字节
00H	代码和高字节
0x03H/0xFFH	结束符（ETX）

*/
void    CMicPro::SendYkSelect(bool argv)
{
    ST_BYTE sendbuf[32];
    sendbuf[0] = 0xEB;
    sendbuf[1] = 0x90;
    sendbuf[2] = 0xEB;
    sendbuf[3] = 0x90;
    sendbuf[4] = 0x02;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = YKSELECT;
    sendbuf[7] = 0x06;
    sendbuf[8] = 0x00;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10] = 0x01;
    sendbuf[11] = argv?0x02:0x01;
    unsigned int sum = 0;
    for(int i = 6;i< 12 ; i++)
        sum += sendbuf[i];
    sendbuf[12] = sum ;
    sendbuf[13] = (sum>>16) ;
    sendbuf[14] = 0x03 ;
    this->Send(sendbuf,15);
}

void    CMicPro::SendYkExecut(bool argv)
{
    ST_BYTE sendbuf[32];
    sendbuf[0] = 0xEB;
    sendbuf[1] = 0x90;
    sendbuf[2] = 0xEB;
    sendbuf[3] = 0x90;
    sendbuf[4] = 0x02;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = YKEXECUT;
    sendbuf[7] = 0x06;
    sendbuf[8] = 0x00;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[10] = 0x01;
    sendbuf[11] = argv?0x02:0x01;
    unsigned int sum = 0;
    for(int i = 6;i< 12 ; i++)
        sum += sendbuf[i];
    sendbuf[12] = sum ;
    sendbuf[13] = (sum>>16) ;
    sendbuf[14] = 0x03 ;
    this->Send(sendbuf,15);
}

void    CMicPro::SendEasyOrder(SENDTYPE sendtype)
{
    ST_BYTE sendbuf[32];
    sendbuf[0] = 0xEB;
    sendbuf[1] = 0x90;
    sendbuf[2] = 0xEB;
    sendbuf[3] = 0x90;
    sendbuf[4] = 0x02;
    sendbuf[5] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[6] = sendtype;
    sendbuf[7] = 0x04;
    sendbuf[8] = 0x00;
    sendbuf[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    unsigned int sum = 0;
    for(int i = 6;i< 10 ; i++)
        sum += sendbuf[i];
    sendbuf[10] = sum ;
    sendbuf[11] = (sum>>16) ;
    sendbuf[12] = 0x03;
    this->Send(sendbuf,13);
}

/*
7、
e2 07 年
0a 月
0b 日
0d 时
39 分
0e 秒
3f 02 毫秒
年月日时分秒毫秒，表示2018年10月11日13点57分14秒575毫秒
*/
void    CMicPro::AnalyzerSOE(ST_BYTE *pbuf)
{
    int nhour,nminte,nmillsecond;
    nhour=pbuf[4];
    nminte=pbuf[5];
    nmillsecond=pbuf[6]+pbuf[7]*256;

 //   this->UpdateValue(itemref.id,bvalue);
    int inf = pbuf[11];
    ST_BYTE bvalue = pbuf[12];
    ST_DataAreaItem itemref;
    if(getDataAreaItem(DataAreaNum::SOEAREA,inf,itemref)<0)  //not found fun/inf to DataAreItems
    {
        ShowMessage("not found fun/inf");
        return;
    }
    ShowMessage("transfer SOE");
    TransferEx (bvalue, itemref.id, nhour, nminte, nmillsecond);
    this->m_pLogger->LogInfo("TransferEx SOE!!!!");
}
void    CMicPro::AnalyzerEvent(ST_BYTE *pbuf)
{
    int nhour,nminte,nmillsecond;
    nhour=pbuf[4];
    nminte=pbuf[5];
    nmillsecond=pbuf[6]+pbuf[7]*256;

 //   this->UpdateValue(itemref.id,bvalue);
    int inf = pbuf[14];
//    ST_BYTE bvalue = pbuf[12];
    ST_DataAreaItem itemref;
    if(getDataAreaItem(DataAreaNum::EVENTAREA,inf,itemref)<0)  //not found fun/inf to DataAreItems
    {
        ShowMessage("not found fun/inf");
        return;
    }
    ShowMessage("transfer event");
    TransferEx (1, itemref.id, nhour, nminte, nmillsecond);
}

int     CMicPro::getDataAreaItem(int fun,int inf,ST_DataAreaItem& itemref)
{
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[fun].itemCount;
    for(ST_INT k = 0; k < itemsize; k++)
    {

        if(inf != this->GetDevice()->GetDeviceInfo()->DataAreas[fun].items[k].addr)
            continue;

        itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[fun].items[k];
        return 1;
    }
    return -1;
}
void    CMicPro::AnalyzerEq(ST_BYTE *pbuf)
{
    float fv1 = pbuf[0] + pbuf[1]*256 + pbuf[2]*256*256 + pbuf[3]*256*256*256;
    float fv2 = pbuf[4] + pbuf[5]*256 + pbuf[6]*256*256 + pbuf[7]*256*256*256;
    float fv3 = pbuf[8] + pbuf[9]*256 + pbuf[10]*256*256 + pbuf[11]*256*256*256;
    float fv4 = pbuf[12] + pbuf[13]*256 + pbuf[14]*256*256 + pbuf[15]*256*256*256;
    this->UpdateValue(10000,fv1);
    this->UpdateValue(10001,fv2);
    this->UpdateValue(10002,fv3);
    this->UpdateValue(10003,fv4);

}

//TODO
/*

接收到的报文解析
1、	eb 90 eb 90 02 报文头
2、	02 起始码
3、	01地址
4、	4d特征码
5、	58 00 报文长度，低字节前、高字节后
6、	01 地址码
7、	11 00 00 12 00 00 13 00 00 14 00 00 18 00 00 19 00 00 1a 00 00 1b 00 00 1c 00 00 1d 00 00
    1e 00 00 1f e8 03 20 00 00 21 00 00 22 00 00 23 00 00 2a 00 00 2b 00 00 2c 00 00 2d 00 00
    2e 00 00 2f 00 00 30 00 00 f0 00 80 f2 00 00 f3 00 00 f4 00 00 f7 00 00
    为报文内容。 其中：11代表测量类型(如:IA、IB、IC、UAB等，详见各装置的信息表)，00 00为相应的测量参数。F0 后的2个字节为开入，共16位，D0位对应开入1，D13对应开入14。测量参数低字节在前、高字节在后。
8、	b8 09为代码和。
9、	fb为结束符，结束符是随机的，不需要检验。

*/
void    CMicPro::AnalyzerYCYX(ST_BYTE *pbuf)
{
    ClearData();
    int dataCount = (pbuf[0] + pbuf[1]*256 - 4) / 3;
    for(int i = 0; i< dataCount ;i++ )
    {
        vector<ST_BYTE> tmpVec ;
        tmpVec.push_back(pbuf[4 + i*3]);
        tmpVec.push_back(pbuf[5 + i*3]);
        m_datamap[pbuf[3 + i*3]] = tmpVec;
    }
/*    char msg[64];
    sprintf(msg,"data count: %d",dataCount);
    ShowMessage(msg);*/
    parseYC();
    parseYX();
}

void    CMicPro::parseYC()
{
    //ShowMessage("1");
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[DataAreaNum::YCAREA].itemCount;
	for(ST_INT k = 0; k < itemsize; k++)
	{
		const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[DataAreaNum::YCAREA].items[k];
		int addr = itemref.addr;
        float fvalue = 0;
		if(itemref.dataLen == 2){
            map<int ,vector<ST_BYTE> >::iterator iter;
            if((iter = m_datamap.find(addr)) != m_datamap.end())
            {
                vector<ST_BYTE> tmpVec = iter->second;
                switch(itemref.dataType) {
                case VALType_Int16:
                    {
                        ST_UINT16 value = 0;
                        memcpy(&value,&tmpVec[0],2);
                        fvalue = (ST_FLOAT)(*((ST_INT16*)&value));;
                    } break;

                case VALType_UInt16:
                    {
                        ST_UINT16 value = 0;
                        memcpy(&value,&tmpVec[0],2);
                        fvalue = (float )value;
                    } break;
                default: break;
                }
            }
            else
            {
                char msg[128];
                sprintf(msg,"not found this addr: %d",addr);
                ShowMessage(msg);
                //ShowMessage("not found this addr");
            }
		}
        else if(itemref.dataLen == 4){

            map<int ,vector<ST_BYTE> >::iterator iter,iter2;
            if((iter = m_datamap.find(addr)) != m_datamap.end())
            {
                iter2 = m_datamap.find(addr+1);

                vector<ST_BYTE> tmpVec = iter->second;
                vector<ST_BYTE> tmpVec2 = iter2->second;

                switch(itemref.dataType) {
                case VALType_Int32:
                    {
                        ST_BYTE bv[4] = {0};
                        memcpy(bv,&tmpVec[0],2);
                        memcpy(&bv[2],&tmpVec2[0],2);

                        ST_UINT32 intval = 0;
                        Memcpy(&intval,bv,4);

                        fvalue = (ST_DOUBLE)(*((ST_INT32*)&intval));;
                    } break;

                case VALType_UInt32:
                    {
                        ST_BYTE bv[4] = {0};
                        memcpy(bv,&tmpVec[0],2);
                        memcpy(&bv[2],&tmpVec2[0],2);

                        ST_INT32 intval = 0;
                        Memcpy(&intval,bv,4);

                        fvalue = (float)intval;
                    } break;
                default: break;
                }
            }
            else
            {
                char msg[128];
                sprintf(msg,"not found this addr: %d",addr);
                ShowMessage(msg);
            }
        }

        if(itemref.coeficient != 0)
        {
            fvalue = fvalue * itemref.coeficient;
        }
        this->UpdateValue(itemref.id, fvalue);
	}

}
/*
void SWAPARRAY(ST_BYTE *buf)
{
    ST_BYTE tmp;

}*/
void    CMicPro::parseYX()
{
    //ShowMessage("2");
    ST_INT itemsize = this->GetDevice()->GetDeviceInfo()->DataAreas[DataAreaNum::YXAREA].itemCount;

    for(ST_INT k = 0; k < itemsize; k++)
    {
        const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[DataAreaNum::YXAREA].items[k];
        int addr = itemref.addr;
        map<int,vector<ST_BYTE> >::iterator iter;

        ST_BYTE bvalue = 0x00;
        if((iter = m_datamap.find(addr)) != m_datamap.end())
        {
            vector<ST_BYTE> tmpVec = iter->second;
            int bitaddr = itemref.beginBit;

            if(bitaddr <= 8)
                bvalue = ((tmpVec.at(0))>>(bitaddr - 1)) & 0x01;
            else
                bvalue = ((tmpVec.at(1))>>(bitaddr - 9)) & 0x01;
        }
        else{
            char msg[128];
            sprintf(msg,"not found this addr: %d",addr);
            ShowMessage(msg);
        }

        this->UpdateValue(itemref.id,bvalue);
    }

}
void    CMicPro::ClearData()
{
    if(m_datamap.empty())
        return ;

    m_datamap.clear();
}

void    CMicPro::TransferEx (ST_BYTE statu, ST_UINT16 addr, ST_BYTE hour, ST_BYTE min, ST_UINT16 msec)
{
	struct tm t_tm;
	DateTime::localtime (time(0), t_tm);

	ProtocolTask task;
	Strcpy(task.taskCmd,"SOE");
	task.isTransfer     = true;
	task.transChannelId = -1;
	task.channelId      = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
	task.deviceId       = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.taskCmdCode    = 0;
	task.taskParamLen   = 14;
	task.taskAddr       = addr;
	task.taskValue      = statu;
	task.taskAddr1      = this->GetDevice()->GetDeviceInfo()->DeviceId;
	task.ignoreBack     = 1;
	task.taskTime       = 1000;
	task.taskParam[0]   = (t_tm.tm_year + 1900) % 256;
	task.taskParam[1]   = (t_tm.tm_year + 1900) / 256;
	task.taskParam[2]   =  t_tm.tm_mon  + 1;
	task.taskParam[3]   =  t_tm.tm_mday;
	task.taskParam[4]   =  hour;
	task.taskParam[5]   =  min;
	task.taskParam[6]   =  msec / 1000;
	task.taskParam[7]   = (msec % 1000) % 256;
	task.taskParam[8]   = (msec % 1000) / 256;
	task.taskParam[9]   = statu;
	task.taskParam[10]  = addr % 256;
	task.taskParam[11]  = addr / 256;
	task.taskParam[12]  = task.taskAddr1 % 256;
	task.taskParam[13]  = task.taskAddr1 / 256;
	Transfer(&task);
}
