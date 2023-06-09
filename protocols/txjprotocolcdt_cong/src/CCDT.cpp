#include "CCDT.h"
#include "datetime.h"

ST_BYTE g_check[256]=
{
	    0x0,0x7,0xE,0x9,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,\
		0x23,0x2A,0x2D,0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,\
		0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,0xE0,0xE7,0xEE,0xE9,0xFC,\
		0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,0x90,\
		0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,\
		0xB3,0xBA,0xBD,0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,\
		0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,0xB7,0xB0,0xB9,0xBE,0xAB,\
		0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,0x27,\
		0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x3,\
		0x4,0xD,0xA,0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,\
		0x61,0x66,0x73,0x74,0x7D,0x7A,0x89,0x8E,0x87,0x80,0x95,0x92,\
		0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,0xF9,0xFE,\
		0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,\
		0xD3,0xD4,0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,\
		0x5F,0x58,0x4D,0x4A,0x43,0x44,0x19,0x1E,0x17,0x10,0x5,0x2,\
		0xB,0xC,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,0x4E,0x49,\
		0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,\
		0x64,0x63,0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x6,0x1,\
		0x8,0xF,0x1A,0x1D,0x14,0x13,0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,\
		0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,0xDE,0xD9,\
		0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,\
		0xF4,0xF3
};

CCDT::CCDT()
{
    SGSendstate=0;
    GjSendstate=0;
}

CCDT::~CCDT()
{
    //dtor
}

void CCDT::Init()
{
}

void CCDT::Uninit()
{

}

CCDT* CreateInstace()
{
    return new CCDT();
}

ST_BOOLEAN	CCDT::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void CCDT::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    readed =0;
}

ST_BOOLEAN	CCDT::OnSend()
{

    if(m_nSendOrder>1) m_nSendOrder = 0;
     if(this->HasTask() && this->GetTask(&m_task))
    {
            if(!strcmp(m_task.taskCmd,"SOE"))
                SendSoe(m_task);
                Thread::SLEEP(20);
                return false;
    }

    Readlimit();
    ReadGJlimit();
    switch(m_nSendOrder)
    {
    case 0:
        SendAllYX();
        break;
    case 1:
        SendAllYC();
        break;
 /*   case 2:
        SendAllYM();
        break;*/
    default:
        SendAllYX();
        break;
    }
    m_nSendOrder++;
    Thread::SLEEP(20);
    return false;
}

ST_BOOLEAN	CCDT::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    return 1;
}

bool CCDT::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
    if (!this->GetDevice())
        return false;
    List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
    if ( !trantables) {
        return false;
    }
    if (trantables->GetCount() <= 0) {
        return false;
    }
    int iter = 0;
    for (; iter < trantables->GetCount(); ++iter)
    {
        if (index == trantables->GetItem(iter)->typeId())
            break;
    }
    if ((table = trantables->GetItem(iter)) == NULL) {
        return false;
    }
    if ((list = table->m_pVarAddrs) == NULL) {
        return false;
    }
    return true;
}

void   CCDT::SendAllYX()
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
    ST_INT bytesum = nCount/32;
    if((nCount%32)!=0) bytesum += 1;
    ST_BYTE pBuffer[1024];
    pBuffer[0] = 0xeb;
    pBuffer[1] = 0x90;
    pBuffer[2] = 0xeb;
    pBuffer[3] = 0x90;
    pBuffer[4] = 0xeb;
    pBuffer[5] = 0x90;
    pBuffer[6] = 0x71;
    pBuffer[7] = 0xf4;
    pBuffer[8] = bytesum;
    pBuffer[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    pBuffer[10]= 0;

    ST_BYTE cha,chb;
    cha=0;
    for(ST_INT crc=0;crc<5;crc++)
    {
        chb=cha^pBuffer[6+crc];
        cha=g_check[chb];
    }
    cha^=0xff;
    pBuffer[11]=cha;

    for(ST_INT i = 0;i< bytesum;i++)
    {
        pBuffer[12+6*i] = 0xf0+i;
        ST_BYTE byRet = 0x01;
        for(ST_INT j=0;j<4;j++)
        {
            pBuffer[12+6*i+j+1] = 0x00;
            for(ST_INT k=0,byRet = 0x01;k<8;k++,byRet=byRet<<1)
            {
                if((32*i+j*8+k)>=nCount) continue;
                ST_DUADDR *stdd = tablelist->GetItem(32*i+j*8+k);
                TRVariable  *TRV = trantable->GetTRVariable(stdd);
    //            m_pLogger->LogDebug("ST_DUADDR = %d, %d, %d, %d",stdd->connect,stdd->device,stdd->addr,stdd->type);
                ST_VARIANT fValue;
                ST_INT nR = GetVariableValueByAddr(*stdd,fValue);
                if(nR<0) fValue.bVal = 0;
                ST_BOOLEAN bValue = fValue.bVal;
                           //m_pLogger->LogDebug("32*i+j*8+k = %d",(32*i+j*8+k));
                          // if((32*i+j*8+k)==(nCount-1))
                          // {
                            //   bValue=Bval;
                               //m_pLogger->LogDebug("bValue = %d",bValue);
                           //}
                if(TRV->Coefficient) bValue = !bValue;
     //           m_pLogger->LogDebug("bValue = %d",bValue);
                if(bValue) pBuffer[12+6*i+j+1] |= byRet;
            }
        }
        cha=0;
        for(ST_INT crc=0;crc<5;crc++)
        {
            chb=cha^pBuffer[12+6*i+crc];
            cha=g_check[chb];
        }
        cha^=0xff;
        pBuffer[17+6*i]=cha;
    }
    this->Send(pBuffer,bytesum*6+12);
}

void   CCDT::SendAllYC()
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YCIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
            ST_INT bytesum = nCount/2;
            if((nCount%2)!=0) bytesum += 1;
            ST_BYTE pBuffer[1024];
            pBuffer[0] = 0xeb;
            pBuffer[1] = 0x90;
            pBuffer[2] = 0xeb;
            pBuffer[3] = 0x90;
            pBuffer[4] = 0xeb;
            pBuffer[5] = 0x90;
            pBuffer[6] = 0x71;
            pBuffer[7] = 0x61;
            pBuffer[8] = bytesum;
            pBuffer[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
            pBuffer[10]= 0;

            ST_BYTE cha,chb;
            cha=0;
            for(ST_INT crc=0;crc<5;crc++)
            {
                chb=cha^pBuffer[6+crc];
                cha=g_check[chb];
            }
            cha^=0xff;
            pBuffer[11]=cha;
            ST_DUADDR *stdd;
            TRVariable  *TRV;
            ST_INT nR;
            ST_VARIANT fValue;
            for(ST_INT i = 0;i< bytesum;i++)
            {
                pBuffer[12+6*i] = 0x00+i;
                ST_FLOAT ff1;
                if((2*i)>=nCount) //continue;
                {
                    ff1=0;
                }
                else
                {
                    stdd = tablelist->GetItem(2*i);
                    TRV = trantable->GetTRVariable(stdd);


                    nR = GetVariableValueByAddr(*stdd,fValue);
                    if(nR<0) fValue.fVal = 0;
                    if(TRV->Coefficient == 0) TRV->Coefficient=1;
                    ff1=fValue.fVal*TRV->Coefficient;
                }

                ST_FLOAT ff2;
                if((2*i+1)>=nCount) //continue;
                {
                    ff2 = 0;
                }
                else
                {
                    stdd = tablelist->GetItem(2*i+1);
                    TRV = trantable->GetTRVariable(stdd);
                    nR = GetVariableValueByAddr(*stdd,fValue);
                    if(nR<0) fValue.fVal = 0;
                    if(TRV->Coefficient == 0) TRV->Coefficient=1;
                    ff2=fValue.fVal*TRV->Coefficient;
                }


                ST_UINT16 wValue1,wValue2;
                if(ff1>=0)
                {
                    wValue1 = (ST_UINT16)(ff1);
                }
                else
                {
                    ff1 = -ff1;
                    wValue1 = (ST_UINT16)(ff1);
                    wValue1 = (0xffff-wValue1)&0x07ff + 0x0800;
                }
                if(ff2>=0)
                {
                    wValue2 = (ST_UINT16)(ff2);
                }
                else
                {
                    ff2 = -ff2;
                    wValue2 = (ST_UINT16)(ff2);
                    wValue2 = (0xffff-wValue2)&0x07ff + 0x0800;
                }
                pBuffer[12+1+i*6] = (ST_BYTE)wValue1;
                pBuffer[12+2+i*6] = (ST_BYTE)(wValue1>>8);
                pBuffer[12+3+i*6] = (ST_BYTE)wValue2;
                pBuffer[12+4+i*6] = (ST_BYTE)(wValue2>>8);
                cha=0;
                for(ST_INT crc=0;crc<5;crc++)
                {
                    chb=cha^pBuffer[12+6*i+crc];
                    cha=g_check[chb];
                }
                cha^=0xff;
                pBuffer[12+5+i*6]=cha;
            }
            this->Send(pBuffer,bytesum*6+12);
}

void    CCDT::SendSoe(ProtocolTask& task)
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return ;
    int32_t list_size = tablelist->GetCount();
    if (list_size <= 0) {
        return ;
    }
    ST_INT  SendSoeNo=0;
    for (; SendSoeNo < list_size; ++SendSoeNo) {
        ST_DUADDR* pda = tablelist->GetItem(SendSoeNo);
        if (task.taskAddr == pda->addr
            && task.taskAddr1 == pda->device)
            break;
    }
    if (SendSoeNo == list_size)
        return;

            ST_INT bytesum;// = nCount/32;
            ST_BYTE pBuffer[1024];
            pBuffer[0] = 0xeb;
            pBuffer[1] = 0x90;
            pBuffer[2] = 0xeb;
            pBuffer[3] = 0x90;
            pBuffer[4] = 0xeb;
            pBuffer[5] = 0x90;
            pBuffer[6] = 0x71;
            pBuffer[7] = 0x26;
            pBuffer[8] = 0x02;
            pBuffer[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
            pBuffer[10]= 0;

            ST_BYTE cha,chb;
            cha=0;
            for(ST_INT crc=0;crc<5;crc++)
            {
                chb=cha^pBuffer[6+crc];
                cha=g_check[chb];
            }
            cha^=0xff;
            pBuffer[11]=cha;
            pBuffer[12]=0x80;
            pBuffer[13]=task.taskParam[7];
            pBuffer[14]=task.taskParam[8];
            pBuffer[15]=task.taskParam[6];
            pBuffer[16]=task.taskParam[5];
            pBuffer[17]=task.taskParam[0];
            pBuffer[18]=0x81;
            pBuffer[19]=task.taskParam[4];
            pBuffer[20]=task.taskParam[3];
            pBuffer[21]=SendSoeNo%256;
            pBuffer[22]=(SendSoeNo/256)|((~(task.taskParam[9]<<7))&0x80);
            //m_pLogger->LogDebug("SendSoeNo = %d",SendSoeNo);
            pBuffer[23]=task.taskParam[0];
            bytesum=0x02;
            for(ST_INT i = 0;i< bytesum;i++)
            {

                cha=0;
                for(ST_INT crc=0;crc<5;crc++)
                {
                    chb=cha^pBuffer[12+6*i+crc];
                    cha=g_check[chb];
                }
                cha^=0xff;
                pBuffer[17+6*i]=cha;
            }
            this->Send(pBuffer,bytesum*6+12);
            memset(&task, 0, sizeof(task));
}
void CCDT::SendAllYM()
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YMIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
            ST_INT bytesum = nCount;
            //if((nCount%2)!=0) bytesum += 1;
            ST_BYTE pBuffer[1024];
            pBuffer[0] = 0xeb;
            pBuffer[1] = 0x90;
            pBuffer[2] = 0xeb;
            pBuffer[3] = 0x90;
            pBuffer[4] = 0xeb;
            pBuffer[5] = 0x90;
            pBuffer[6] = 0x71;
            pBuffer[7] = 0x85;
            pBuffer[8] = bytesum;
            pBuffer[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
            pBuffer[10]= 0;

            ST_BYTE cha,chb;
            cha=0;
            for(ST_INT crc=0;crc<5;crc++)
            {
                chb=cha^pBuffer[6+crc];
                cha=g_check[chb];
            }
            cha^=0xff;
            pBuffer[11]=cha;

            for(ST_INT i = 0;i< bytesum;i++)
            {
                if(i>=nCount) break;
                pBuffer[12+6*i] = 0xa0+i;
                ST_DUADDR *stdd = tablelist->GetItem(i);
                TRVariable  *TRV = trantable->GetTRVariable(stdd);
                ST_VARIANT fValue;
                ST_INT nR = GetVariableValueByAddr(*stdd,fValue);
                if(nR<0) fValue.dtVal = 0;
                if(TRV->Coefficient == 0) TRV->Coefficient=1;
                ST_DOUBLE ff1 = fValue.dtVal*TRV->Coefficient;
                ST_UINT32 wValue1;
                wValue1 = (ST_UINT32)(ff1);
                pBuffer[12+1+i*6] = (ST_BYTE)wValue1;
                pBuffer[12+2+i*6] = (ST_BYTE)(wValue1>>8);
                pBuffer[12+3+i*6] = (ST_BYTE)(wValue1>>16);
                pBuffer[12+4+i*6] = (ST_BYTE)(wValue1>>24);
                cha=0;
                for(ST_INT crc=0;crc<5;crc++)
                {
                    chb=cha^pBuffer[12+6*i+crc];
                    cha=g_check[chb];
                }
                cha^=0xff;
                pBuffer[12+5+i*6]=cha;
            }
            this->Send(pBuffer,bytesum*6+12);
}

void    CCDT::Readlimit()
{
    Bval=0;
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (SGIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
            for(ST_INT crc=0;crc<nCount;crc++)
                {
                        ST_DUADDR *stdd = tablelist->GetItem(crc);
                      //  TRVariable  *TRV = trantable->GetTRVariable(stdd);
 //                       m_pLogger->LogDebug("ST_DUADDR = %d, %d, %d, %d",stdd->connect,stdd->device,stdd->addr,stdd->type);
                        ST_VARIANT fValue;
                        ST_INT nR = GetVariableValueByAddr(*stdd,fValue);
                        if(nR<0) fValue.bVal = 0;
                        ST_BOOLEAN bValue = fValue.bVal;
                        if(bValue)
                        {

                            Bval=1;

                            this->UpdateValue(0,(ST_FLOAT)Bval);
                            //m_pLogger->LogDebug("BNvar.bVal %d",BNvar.bVal);
                            if((SGSendstate==0x00)&&Bval==0x01)
                            {
                                    SGSendstate=1;
                                    SendGJSoe(0,Bval);
                            }
                            return;
                            break;
                        }

                }
                Bval=0;
                if((SGSendstate==0x01)&&Bval==0x00)
                {
                        SGSendstate=0;
                        SendGJSoe(0,Bval);
                }
                this->UpdateValue(0,(ST_FLOAT)Bval);
                return;
}

void    CCDT::ReadGJlimit()
{
    Bval=0;
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (GJIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
            for(ST_INT crc=0;crc<nCount;crc++)
                {
                        ST_DUADDR *stdd = tablelist->GetItem(crc);
                      //  TRVariable  *TRV = trantable->GetTRVariable(stdd);
 //                       m_pLogger->LogDebug("ST_DUADDR = %d, %d, %d, %d",stdd->connect,stdd->device,stdd->addr,stdd->type);
                        ST_VARIANT fValue;
                        ST_INT nR = GetVariableValueByAddr(*stdd,fValue);
                        if(nR<0) fValue.bVal = 0;
                        ST_BOOLEAN bValue = fValue.bVal;
                        if(bValue)
                        {

                            Bval=1;

                            this->UpdateValue(1,(ST_FLOAT)Bval);
                            //m_pLogger->LogDebug("BNvar.bVal %d",BNvar.bVal);
                            if((GjSendstate==0x00)&&Bval==0x01)
                            {
                                    GjSendstate=1;
                                    SendGJSoe(1,Bval);
                            }
                            return;
                            break;
                        }

                }
                Bval=0;
                this->UpdateValue(1,(ST_FLOAT)Bval);
                 if((GjSendstate==0x01)&&Bval==0x00)
                    {
                            GjSendstate=0;
                            SendGJSoe(1,Bval);
                    }
                return;
}

void    CCDT::SendGJSoe(ST_BYTE SendSoeNo,ST_BOOLEAN BVal)
{
    TransferTable *trantable   = NULL;
    List<ST_DUADDR> *tablelist = NULL;

    if (! CheckTransferTableExist (YXIndex, trantable, tablelist))
        return ;
    int32_t nCount = tablelist->GetCount();
    if (nCount <= 0) {
        return ;
    }
             for(ST_INT indexno=0;indexno<nCount; indexno++)
                {
                    ST_DUADDR *stdd = tablelist->GetItem(indexno);
                    TRVariable  *TRV = trantable->GetTRVariable(stdd);
                    if(stdd->addr==(SendSoeNo))
                    {
                        SendSoeNo=indexno;
                        break;
                    }
                }
            struct tm tm_now;
            DateTime::localtime (time(0), tm_now);

            ST_INT bytesum;// = nCount/32;
            ST_BYTE pBuffer[1024];
            pBuffer[0] = 0xeb;
            pBuffer[1] = 0x90;
            pBuffer[2] = 0xeb;
            pBuffer[3] = 0x90;
            pBuffer[4] = 0xeb;
            pBuffer[5] = 0x90;
            pBuffer[6] = 0x71;
            pBuffer[7] = 0x26;
            pBuffer[8] = 0x02;
            pBuffer[9] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
            pBuffer[10]= 0;

            ST_BYTE cha,chb;
            cha=0;
            for(ST_INT crc=0;crc<5;crc++)
            {
                chb=cha^pBuffer[6+crc];
                cha=g_check[chb];
            }
            cha^=0xff;
            pBuffer[11]=cha;
            pBuffer[12]=0x80;
            ST_INT  mulsecond= (tm_now.tm_sec)*10;
            pBuffer[13]=mulsecond%256;
            pBuffer[14]=mulsecond/256;
            pBuffer[15]=tm_now.tm_sec;
            pBuffer[16]=tm_now.tm_min;
            pBuffer[17]=0;
            pBuffer[18]=0x81;
            pBuffer[19]=tm_now.tm_hour;
            pBuffer[20]=tm_now.tm_mday;
            pBuffer[21]=SendSoeNo%256;
            pBuffer[22]=(SendSoeNo/256)|((~(BVal<<7))&0x80);
           // m_pLogger->LogDebug("SendSoeNo = %d",SendSoeNo);
            pBuffer[23]=0;
            bytesum=0x02;
            for(ST_INT i = 0;i< bytesum;i++)
            {

                cha=0;
                for(ST_INT crc=0;crc<5;crc++)
                {
                    chb=cha^pBuffer[12+6*i+crc];
                    cha=g_check[chb];
                }
                cha^=0xff;
                pBuffer[17+6*i]=cha;
            }
            this->Send(pBuffer,bytesum*6+12);
}
