// DL645.cpp: implementation of the CDL645 class.
//
//////////////////////////////////////////////////////////////////////

#include "DL645.h"
#include "Device.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "http_server.h"



void ToBCD(int number, uint8_t *bcdArray,int len) {
    int i;
    for (i = 0; i < len; i++) {
        bcdArray[i] = number % 10;
        number /= 10;
        bcdArray[i] |= (number % 10) << 4;
        number /= 10;
    }
}


inline ST_BYTE get_check_sum(ST_BYTE* pbuf,ST_BYTE len)
{
    ST_BYTE bySum = 0x00;
    for(int i = 0; i < len; ++i)
    {
        bySum += pbuf[i];
    }
    return  bySum;
}

uint64_t CDL645::reverse_bytes(ST_BYTE *bcdArray, ST_BYTE len)
{
    //1.数据区-0x33
    for(int i=0; i<len; i++)
    {
        bcdArray[i] -= 0x33;
    }

    //2.字节数组逆序
    for (ST_BYTE i = 0; i < len / 2; i++)
    {
        ST_BYTE temp = bcdArray[i];
        bcdArray[i] = bcdArray[len - i - 1];
        bcdArray[len - i - 1] = temp;
    }

    //BCD解析int
    int i;
    int number = 0;
    for (i = 0; i < len; i++) {
        number *= 100;
        number += ((bcdArray[i] >> 4) & 0x0F) * 10 + (bcdArray[i] & 0x0F);
    }

    return number;
}



Protocol * CreateInstace()
{
    return new CDL645();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDL645::CDL645()
{
    m_readIndex = 0;
    nRechargeStep = 0;
    m_nRechargeValue = 0;
    memset (m_addrarea, 0, sizeof(m_addrarea));
    memset (&m_curTask,0,sizeof(m_curTask));
    isYK = false;

}

CDL645::~CDL645()
{
}

void CDL645::Init()
{

}

void CDL645::Uninit()
{
}

bool CDL645::IsSupportEngine(ST_INT engineType)
{
    return true;
}

void CDL645::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
    readed = 0;
    if(this->GetCurPort())
    {

        ST_INT len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);

        if(len < 12)
        {
            ShowMessage ("Insufficient data length");
            return;
        }

        ST_INT star = 0;
        for(; star < len; ++star)
        {
            if(pbuf[star] == 0x68)
                break;
        }
        if(len == star)
        {
            ShowMessage ("Garbled code, clear buffer.");
            this->GetCurPort()->Clear();
            return;
        }
        if(star > 0)
        {
            this->GetCurPort()->ReadBytes(pbuf, star);
        }

        if (this->GetDevice()->GetDeviceInfo()->DataAreasCount <= 0)
        {
            ShowMessage("No Point Table.");
            this->GetCurPort()->Clear();
            return;
        }

        len = this->GetCurPort()->PickBytes(pbuf, 12, 2000);
        if((pbuf[0] == 0x68) && (pbuf[7] == 0x68))
        {
            ST_INT ndatalen = pbuf[9] + 12;
            if(this->GetCurPort()->ReadBytes(pbuf, ndatalen) == ndatalen)
            {
                if (memcmp(pbuf + 1, m_addrarea, sizeof(m_addrarea)))
                {
                    this->ShowMessage("Address not match.");
                    return;
                }
                if (get_check_sum(pbuf, ndatalen - 2) == pbuf[ndatalen - 2])
                {
                    readed = ndatalen;
                    return;
                }
                else
                {
                    ShowMessage("Check error!");
                    this->ShowRecvFrame(pbuf,len);
                    this->GetCurPort()->Clear();
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
    else
        isYK = false;
}

bool CDL645::OnSend()
{

    //m_pLogger->GetInstance()->LogDebug("taskCmd = %s, taskCmdCode = %d, param = %s,paramLen = %d",m_curTask.taskCmd,m_curTask.taskCmdCode,m_curTask.taskParam,m_curTask.taskParamLen);
    if (nRechargeStep==2)
    {
        YK10(m_nRechargeValue);
        nRechargeStep = 0;
        m_nRechargeValue = 0;
        return true;
    }
    if (this->HasTask() && this->GetTask(&m_curTask))
    {
        m_pLogger->GetInstance()->LogDebug("parameters in DL645:");
        m_pLogger->GetInstance()->LogDebug("taskCmd=%s,timeout=%d",m_curTask.taskCmd,m_curTask.timeOut);
        if (!strcmp(m_curTask.taskCmd, "devicecontrol"))
        {
            if (m_curTask.taskCmdCode == 1)
            {
                SendYK(m_curTask.taskValue);
                isYK = true;
            }
            return true;
        }
        else if(!strcmp (m_curTask.taskCmd, "recharge"))
        {
            m_nRechargeValue = m_curTask.timeOut;
            m_pLogger->GetInstance()->LogDebug("rechargeValue=%d,deviceID = %d",m_nRechargeValue,this->GetDeviceId());
            YK4();
            nRechargeStep = 1;
            //YK10(m_curTask.timeOut);
            return true;
        }
    }




    //YK查詢剩電量放在YC區


    switch(m_readIndex)
    {
    case 0:
        ReadData(0x00000000);
        break; // 当前组合有功总电能
    case 1:
        ReadData(0x00010000);
        break; // 当前正向有功总电能 1
    case 2:
        ReadData(0x00020000);
        break; // 当前反向有功总电能
    case 3:
        ReadData(0x00030000);
        break; // 组合无功1总电能
    case 4:
        ReadData(0X00040000);
        break; // 组合无功2总电能
    case 5:
        ReadData(0x00050000);
        break; // 第一象限无功总电能
    case 6:
        ReadData(0x00060000);
        break; // 第二象限无功总电能
    case 7:
        ReadData(0x00070000);
        break; // 第三象限无功总电能
    case 8:
        ReadData(0x00080000);
        break; // 第四象限无功总电能
    case 9:
        ReadData(0x0201ff00);
        break; // 电压
    case 10:
        ReadData(0x0202ff00);
        break; // 电流
    case 11:
        ReadData(0x0203ff00);
        break; // 有功功率
    case 12:
        ReadData(0x0204ff00);
        break; // 无功功率
    case 13:
        ReadData(0x0205FF00);
        break; // 视在功率
    case 14:
        ReadData(0x0206ff00);
        break; // 功率因数
    case 15:
        ReadData(0x0207FF00);
        break; // 相角
    case 16:
        ReadData(0x040005FF);
        break; // 电表运行状态字
    case 17:
        ReadData(0x02800001);
        break; // 当前零序电流
    case 18:
        ReadData(0x01010000);
        break; // 当前最大需量
    case 19:
        ReadData(0x01010001);
        break; // 上一日正向有功最大需量 11

    case 20:
        ReadData(0x00010100);
        break; // 正向有功费率1电能 2
    case 21:
        ReadData(0x00010200);
        break; // 正向有功费率2电能 3
    case 22:
        ReadData(0x00010300);
        break; // 正向有功费率3电能 4
    case 23:
        ReadData(0x00010400);
        break; // 正向有功费率4电能 5
    case 24:
        ReadData(0x00010001);
        break; // (上1结算日)正向有功总电能 6
    case 25:
        ReadData(0x00010101);
        break; // (上1结算日)正向有功费率1电能 7
    case 26:
        ReadData(0x00010201);
        break; // (上1结算日)正向有功费率2电能 8
    case 27:
        ReadData(0x00010301);
        break; // (上1结算日)正向有功费率3电能 9
    case 28:
        ReadData(0x00010401);
        break; // (上1结算日)正向有功费率4电能 10
    case 29:
        YK4();
        break; // (上1结算日)正向有功费率4电能 10
    default:
        break;
    }
    m_readIndex = ((++m_readIndex) % 30); // 19+1
    return true;
}

bool CDL645::OnProcess (ST_BYTE* pbuf, ST_INT len)
{
    if (isYK)
    {
        m_curTask.taskResult.resultCode = (pbuf[8] & 0x40)? 1:0;
        m_curTask.isTransfer = 1;
        Transfer(&m_curTask);
        memset(&m_curTask,0,sizeof(m_curTask));
        isYK = false;
        m_readIndex = 16;
        return true;
    }

    //報文類型B
    if(pbuf[9]==0x46)
    {
        uint64_t userNo = reverse_bytes(pbuf+10,6);
        this->UpdateValue(400, userNo);
        int totalCharge = reverse_bytes(pbuf+19,4)/100;
        this->UpdateValue(401, totalCharge);
        int remainCharge = reverse_bytes(pbuf+23,4)/100;
        this->UpdateValue(402, remainCharge);
        int overdrawCharge = reverse_bytes(pbuf+27,4)/100;
        this->UpdateValue(403,overdrawCharge);
        uint64_t lastDate = reverse_bytes(pbuf+31,5);
        this->UpdateValue(404, lastDate);
        //購電次數
        uint16_t chargeCount = reverse_bytes(pbuf+36,2);
        chargeArray[this->GetDeviceId()].chargeCount = chargeCount;
        this->UpdateValue(405, chargeCount);
        m_pLogger->LogDebug("length of pbuf = %d",len);
        m_pLogger->LogDebug("pbuf[36] = %02x ",pbuf[36]);
        m_pLogger->LogDebug("pbuf[37] = %02x ",pbuf[37]);
        //最近一次購電量
        uint32_t lastRechargeQuantity = reverse_bytes(pbuf+38,4)/100;
        this->UpdateValue(406, (int)lastRechargeQuantity);
        //總購電量
        chargeArray[this->GetDeviceId()].totalChargeValue = reverse_bytes(pbuf+42,4)/100;
        //聲光報警電量
        uint32_t acousticQuantity = reverse_bytes(pbuf+46,4)/100;
        chargeArray[this->GetDeviceId()].acousticQuantity = acousticQuantity;
        this->UpdateValue(407, (int)acousticQuantity);
        //拉渣報警電量
        uint32_t switchQuantity = reverse_bytes(pbuf+50,4)/100;
        chargeArray[this->GetDeviceId()].switchQuantity = switchQuantity;
        this->UpdateValue(408, (int)switchQuantity);
        //囤積限量
        uint32_t storage = reverse_bytes(pbuf+54,4)/100;
        chargeArray[this->GetDeviceId()].storage = storage;
        this->UpdateValue(409, (int)storage);
        //賒欠限量
        uint32_t credit = reverse_bytes(pbuf+58,4)/100;
        chargeArray[this->GetDeviceId()].credit = credit;
        this->UpdateValue(410, (int)credit);
        //縣容功率
        uint32_t limitPower = reverse_bytes(pbuf+62,3)/100;
        chargeArray[this->GetDeviceId()].limitPower = limitPower;
        this->UpdateValue(411, (int)limitPower);
        //當前功率
        uint32_t curPower = reverse_bytes(pbuf+65,3)/100;
        this->UpdateValue(412, (int)curPower);
        //縣容方式
        uint8_t limitType = reverse_bytes(pbuf+68,1)/100;
        chargeArray[this->GetDeviceId()].limitType = limitType;
        this->UpdateValue(413, limitType);
        //運行狀態字
        uint16_t value801 = reverse_bytes(pbuf+70,2)/100;
        this->UpdateValue(414, value801);
        //系統密碼
        uint32_t sysPwd = reverse_bytes(pbuf+72,3);
        this->UpdateValue(415, (int)sysPwd);
        //通訊地址
        uint64_t commAddress = reverse_bytes(pbuf+74,6);
        this->UpdateValue(416, commAddress);

        if(nRechargeStep == 1)
        {
            nRechargeStep=2;
        }
        return true;
    }

    /*
    if(pbuf[9]==0x46)
    {
        int remainCharge = reverse_bytes(pbuf+23,4);
        m_pLogger->GetInstance()->LogDebug("remainCharge = %d",remainCharge);
        this->UpdateValue(402, remainCharge);
        return true;
    }*/


    if(pbuf[8] == 0x91)
    {
        unsigned long datatype = 0;
        memcpy(&datatype, pbuf + 10, sizeof(datatype));
        switch (datatype)
        {
        case 0x00000000:
        case 0x33333333:
        {
            //const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[0];
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0x70) >> 4) * 10000000)*0.01;

            if ( (pbuf[17]-0x33) & 0x80 )
            {
                if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                    fvalue = 799999.99;
                fvalue = -1.0 * fvalue;
            }
            else
            {
                if (fvalue > 799999.99) // 取值范围：0.00～799999.99。
                    fvalue = 799999.99;
            }
            //if (itemref.coeficient != 0)
            //    fvalue = fvalue * itemref.coeficient;
            this->UpdateValue(0, fvalue);
        }
        break;
        case 0x00010000:
        case 0x33343333:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(1, fvalue);
        }
        break;
        case 0x00020000:
        case 0x33353333:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(2, fvalue);
        }
        break;
        case 0x00030000:
        case 0x33363333:   // 组合无功1总电能 可能带负值
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;

            if ((pbuf[17] - 0x33) & 0x80)
            {
                if (fvalue > 799999.99)
                    fvalue = 799999.99;
                fvalue = -1.0 * fvalue;
            }
            else
            {
                if (fvalue > 799999.99)
                    fvalue = 799999.99;
            }
            this->UpdateValue(3, fvalue);
        }
        break;
        case 0x00040000:
        case 0x33373333:   // 组合无功2总电能 可能带负值
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0x70)>>4)*10000000)*0.01;


            if ((pbuf[17] - 0x33) & 0x80)
            {
                if (fvalue > 799999.99)
                    fvalue = 799999.99;
                fvalue = -1.0 * fvalue;
            }
            else
            {
                if (fvalue > 799999.99)
                    fvalue = 799999.99;
            }
            this->UpdateValue(4, fvalue);
        }
        break;
        case 0x00050000:
        case 0x33383333:    //
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
            this->UpdateValue(5, fvalue);
        }
        break;
        case 0x00060000:
        case 0x33393333:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
            this->UpdateValue(6, fvalue);
        }
        break;
        case 0x00070000:
        case 0x333A3333:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
            this->UpdateValue(7, fvalue);
        }
        break;
        case 0x00080000:
        case 0x333B3333:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000 +
                                   ((pbuf[17]-0x33)&0x0f)*1000000 + (((pbuf[17]-0x33)&0xf0)>>4)*10000000)*0.01;
            this->UpdateValue(8, fvalue);
        }
        break;



        case 0x0201FF00:
        case 0x35343233:
        {
            float fvalue = 0;
            for(int i = 0; i < 3; ++i)
            {
                fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0xF0)>>4)*1000)*0.1;
                this->UpdateValue(i +  9, fvalue);
            }
        }
        break;

        case 0x0202FF00:
        case 0x35353233:
        {
            float fvalue = 0;
            for(int i = 0; i < 3; ++i)
            {
                fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                                 ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.001;

                if ((pbuf[16+i*3]-0x33) & 0x80)
                {
                    if (fvalue > 799.999)
                        fvalue = 799.999;
                    fvalue = -1.0 * fvalue;
                }
                else
                {
                    if (fvalue > 799.999)
                        fvalue = 799.999;
                }
                this->UpdateValue(i +  12, fvalue);
            }
        }
        break;

        case 0x0203FF00:
        case 0x35363233:
        {
            float fvalue = 0;
            for(int i = 0; i < 4; ++i)
            {
                fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                                 ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;

                if ((pbuf[16+i*3] -0x33) & 0x80)
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                    fvalue = -1.0 * fvalue;
                }
                else
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                }
                this->UpdateValue(i +  15, fvalue);
            }
        }
        break;

        case 0x0204FF00:
        case 0x35373233:
        {
            float fvalue = 0;
            for(int i = 0; i < 4; ++i)
            {
                fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                                 ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;


                if ((pbuf[16+i*3] -0x33) & 0x80)
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                    fvalue = -1.0 * fvalue;
                }
                else
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                }

                this->UpdateValue(i + 19, fvalue);
            }
        }
        break;
        case 0x0205FF00:
        case 0x35383233:
        {
            float fvalue = 0;
            for(int i = 0; i < 4; ++i)
            {
                fvalue = (float)(((pbuf[14+i*3]-0x33)&0x0f) + (((pbuf[14+i*3]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*3]-0x33)&0x0f)*100 + (((pbuf[15+i*3]-0x33)&0xf0)>>4)*1000 +
                                 ((pbuf[16+i*3]-0x33)&0x0f)*10000 + (((pbuf[16+i*3]-0x33)&0x70)>>4)*100000)*0.0001;

                if ((pbuf[16+i*3] -0x33) & 0x80)
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                    fvalue = -1.0 * fvalue;
                }
                else
                {
                    if (fvalue > 79.9999)
                        fvalue = 79.9999;
                }

                this->UpdateValue(i + 23, fvalue);
            }
        }
        break;
        case 0x0206FF00:
        case 0x35393233:
        {
            float fvalue = 0;
            for (int i = 0; i < 4; ++i)
            {
                fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f) + (((pbuf[14+i*2]-0x33)&0xf0)>>4)*10 +
                                 ((pbuf[15+i*2]-0x33)&0x0f)*100 + (((pbuf[15+i*2]-0x33)&0x70)>>4)*1000)*0.001;

                if ((pbuf[15+i*2]-0x33) & 0x80)
                {
                    if (fvalue > 1)
                        fvalue = 1.000;
                    fvalue = -1.0 * fvalue;
                }
                else
                {
                    if (fvalue > 1)
                        fvalue = 1.000;
                }
                this->UpdateValue(i + 27, fvalue);
            }
        }
        break;

        case 0x0207FF00:
        case 0x353A3233:
        {
            float fvalue = 0;
            for (int i = 0; i < 3; ++i)
            {
                fvalue = (float)(((pbuf[14+i*2]-0x33)&0x0f)*0.1 + (((pbuf[14+i*2]-0x33)&0xf0)>>4) +
                                 ((pbuf[15+i*2]-0x33)&0x0f)*10 + (((pbuf[15+i*2]-0x33)&0xf0)>>4)*100);
                if (fvalue > 360)
                    fvalue = 360;
                this->UpdateValue(i + 31, fvalue);
            }
        }
        break;
        case 0x02800001:
        case 0x35B33334:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0x70)>>4)*100000)*0.001;
            if ((pbuf[16]-0x33) & 0x80)
            {
                if (fvalue > 799.999)
                    fvalue = 799.999;
                fvalue = -1.0 * fvalue;
            }
            else
            {
                if (fvalue > 799.999)
                    fvalue = 799.999;
            }
            this->UpdateValue(34, fvalue);
        }
        break;

        case 0x01010000:
        case 0x34343333:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000)*0.0001;
            this->UpdateValue(35, fvalue);
        }
        break;
        case 0x01010001:
        case 0x34343334:
        {
            float fvalue = (float)(((pbuf[14]-0x33)&0x0f) + (((pbuf[14]-0x33)&0xf0)>>4)*10 +
                                   ((pbuf[15]-0x33)&0x0f)*100 + (((pbuf[15]-0x33)&0xf0)>>4)*1000 +
                                   ((pbuf[16]-0x33)&0x0f)*10000 + (((pbuf[16]-0x33)&0xf0)>>4)*100000)*0.0001;
            this->UpdateValue(36, fvalue);
        }
        break;

        case 0x040005FF:
        case 0x37333832:
        {
            // 七个状态字
            for (int i=0; i<7; ++i)
            {
                uint8_t bith = pbuf[15+i*2] - 0x33;
                uint8_t bitl = pbuf[14+i*2] - 0x33;
                if(i == 0)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x02)? 1:0 ;
                    this->UpdateValue(50,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(51,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(52,value);
                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(53,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(54,value);
                }
                else if (i == 1)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(100,value);
                    value = (bitl & 0x02)? 1:0;
                    this->UpdateValue(101,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(102,value);

                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(103,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(104,value);
                    value = (bitl & 0x40)? 1:0;
                    this->UpdateValue(105,value);
                }
                else if (i == 2)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(150,value);
                    value = (bitl & 0x60);
                    value = value >> 1;
                    this->UpdateValue(151,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(152,value);
                    value = (bitl & 0x10)? 0:1;
                    this->UpdateValue(153,value);
                }
                else if (i == 3)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(200,value);
                    value = (bitl & 0x02)? 1:0;
                    this->UpdateValue(201,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(202,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(203,value);

                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(204,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(205,value);
                    value = (bitl & 0x40)? 1:0;
                    this->UpdateValue(206,value);
                    value = (bitl & 0x80)? 1:0;
                    this->UpdateValue(207,value);

                    value = (bith & 0x01)? 1:0;
                    this->UpdateValue(208,value);
                }
                else if (i == 4)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(250,value);
                    value = (bitl & 0x02)? 1:0;
                    this->UpdateValue(251,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(252,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(253,value);

                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(254,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(255,value);
                    value = (bitl & 0x40)? 1:0;
                    this->UpdateValue(256,value);
                    value = (bitl & 0x80)? 1:0;
                    this->UpdateValue(257,value);

                    value = (bith & 0x01)? 1:0;
                    this->UpdateValue(258,value);
                }
                else if (i == 5)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(300,value);
                    value = (bitl & 0x02)? 1:0;
                    this->UpdateValue(301,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(302,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(303,value);

                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(304,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(305,value);
                    value = (bitl & 0x40)? 1:0;
                    this->UpdateValue(306,value);
                    value = (bitl & 0x80)? 1:0;
                    this->UpdateValue(307,value);

                    value = (bith & 0x01)? 1:0;
                    this->UpdateValue(308,value);
                }
                else if (i == 6)
                {
                    ST_BYTE value = 0;
                    value = (bitl & 0x01)? 1:0;
                    this->UpdateValue(350,value);
                    value = (bitl & 0x02)? 1:0;
                    this->UpdateValue(351,value);
                    value = (bitl & 0x04)? 1:0;
                    this->UpdateValue(352,value);
                    value = (bitl & 0x08)? 1:0;
                    this->UpdateValue(353,value);

                    value = (bitl & 0x10)? 1:0;
                    this->UpdateValue(354,value);
                    value = (bitl & 0x20)? 1:0;
                    this->UpdateValue(355,value);
                    value = (bitl & 0x40)? 1:0;
                    this->UpdateValue(356,value);
                }
            }
        }
        break;
        case 0x00010100:
        case 0x33343433:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(37, fvalue);
        }
        break;
        case 0x00010200:
        case 0x33343533:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(38, fvalue);
        }
        break;
        case 0x00010300:
        case 0x33343633:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(39, fvalue);
        }
        break;
        case 0x00010400:
        case 0x33343733:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(40, fvalue);
        }
        break;
        case 0x00010001:
        case 0x33343334:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(41, fvalue);
        }
        break;
        case 0x00010101:
        case 0x33343434:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(42, fvalue);
        }
        break;
        case 0x00010201:
        case 0x33343534:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(43, fvalue);
        }
        break;
        case 0x00010301:
        case 0x33343634:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(44, fvalue);
        }
        break;
        case 0x00010401:
        case 0x33343734:
        {
            float fvalue = (float)(((pbuf[14] - 0x33) & 0x0f) + (((pbuf[14] - 0x33) & 0xf0) >> 4) * 10 +
                                   ((pbuf[15] - 0x33) & 0x0f) * 100 + (((pbuf[15] - 0x33) & 0xf0) >> 4) * 1000 +
                                   ((pbuf[16] - 0x33) & 0x0f) * 10000 + (((pbuf[16] - 0x33) & 0xf0) >> 4) * 100000 +
                                   ((pbuf[17] - 0x33) & 0x0f) * 1000000 + (((pbuf[17] - 0x33) & 0xf0) >> 4) * 10000000)*0.01;
            this->UpdateValue(45, fvalue);
        }
        break;
        default:
        {
            this->ShowMessage("UnKown data id.");
        }
        break;
        }
    }
    else if (pbuf[8] == 0xD1 && pbuf[9] == 0x01)
    {
        switch(pbuf[10]-0x33)
        {
        case 0x01:
            this->ShowMessage("Device error response，the reason：other.");
            break;
        case 0x02:
            this->ShowMessage("Device error response，the reason：no request data.");
            break;
        case 0x04:
            this->ShowMessage("Device error response，the reason：no access.");
            break;
        case 0x08:
            this->ShowMessage("Device error response，the reason：baudrate can't change.");
            break;
        case 0x10:
            this->ShowMessage("Device error response，the reason：year time zone overflow.");
            break;
        case 0x20:
            this->ShowMessage("Device error response，the reason：day tiem zone overflow.");
            break;
        case 0x40:
            this->ShowMessage("Device error response，the reason：rate overflow.");
            break;
        default:
            this->ShowMessage("Device error response，the reason：undefined.");
            break;
        }
    }
    return true;
}

void    CDL645::ReadData(ST_UINT32 wAddr) //参数由高到低，写出由低到高  主站请求帧
{
    ST_BYTE sendbuf[32] = {0};
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    sendbuf[0] = 0x68;


    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 01, m_addrarea, sizeof(m_addrarea));

    sendbuf[7] = 0x68;
    sendbuf[8] = 0x11;
    sendbuf[9] = 0x04;

    ST_UINT16 wTempH = (wAddr & 0xffff0000) >> 16;
    ST_UINT16 wTempL =  wAddr & 0x0000ffff;
    sendbuf[10] = ( wTempL&0x00ff)    +0x33;
    sendbuf[11] = ((wTempL&0xff00)>>8)+0x33;
    sendbuf[12] = ( wTempH&0x00ff)    +0x33;
    sendbuf[13] = ((wTempH&0xff00)>>8)+0x33;

    ST_BYTE bySum = 0x00;
    for(int i = 0; i < 14; ++i)
    {
        bySum += sendbuf[i];
    }
    sendbuf[14] = bySum;
    sendbuf[15] = 0x16;
    this->Send(sendbuf, 16);
}


void    CDL645::SendYK(bool YKBit)
{
    time_t stamp=time(NULL)+24*60*60;
    struct tm *t=localtime(&stamp);


    switch(m_curTask.taskCmdCode)
    {
    case 1:
        YK1();  //清零总命令
        break;
    case 2:
        YK2();  //用户清零命令
        break;
    case 10:
        YK3();  //测试卡命令
        break;
    case 4:
        YK4();  //查询卡命令
        break;
    case 7:
        YK7();  //预置卡命令
        break;
    case 8:
        YK8();  //管理卡命令
        break;
    case 9:
        YK9();  //开户卡命令
        break;
    case 11:
        YK11(); //退费卡命令
        break;
    }
}

void CDL645::GetRand(ST_BYTE* buf)
{
    srand(time(NULL));
    buf[10] = (rand()%100+1);
    buf[11] = (rand()%100+1);
    buf[12] = (rand()%100+1);
    buf[13] = (rand()%100+1);
    buf[14] = (rand()%100+1);
    buf[15] = (rand()%100+1);
    buf[16] = (ST_BYTE)((ST_BYTE)((ST_BYTE)(buf[10]+buf[12]+buf[13])^(ST_BYTE)(buf[11]+buf[12]+buf[15]))+0x44);
    buf[17] = (ST_BYTE)((ST_BYTE)(buf[11]+buf[13]+buf[14]+buf[15])+(ST_BYTE)(buf[11]^buf[12]^buf[14]));
    buf[18] = (ST_BYTE)((ST_BYTE)(buf[12]+buf[13]+buf[14])+(ST_BYTE)(buf[10]^buf[11]^buf[12]^buf[14]));
    buf[19] = (ST_BYTE)(((ST_BYTE)(buf[10]+buf[15])^(ST_BYTE)(buf[12]+buf[13]+buf[14]+buf[15]))+0XA3);
}

ST_BYTE CDL645::GetYZ2(ST_BYTE* buf)
{
    long temp = 0;
    for(int i = 0; i<buf[9]-1; i++)
    {
        temp+=buf[10+i];
    }
    return (ST_BYTE)((ST_BYTE)((ST_BYTE)temp^0x36)+0x99);
}


//总清零命令
void CDL645::YK1()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x0F;

    //10字节验证加密
    GetRand(sendbuf);

    sendbuf[21] = 0XA0;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X03;
    //命令数据域
    sendbuf[23] = 0X00;
    sendbuf[24] = 0x00;
    //命令验证码算法YZ2
    sendbuf[25] = (ST_BYTE)(GetYZ2(sendbuf)+0x33);

    //校验码
    sendbuf[26] = CSCheck(sendbuf,26);
    //结束码
    sendbuf[27] = 0x16;
    this->Send(sendbuf,28);
}

//用户清零命令
void CDL645::YK2()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x18;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XA1;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X0C;
    //命令数据域11字节（00...00 + 3字节系统密码）
    //memcpy (sendbuf + 23, sysPwd, sizeof(sysPwd));  //还没写好
    //命令验证码算法YZ2
    sendbuf[33] = GetYZ2(sendbuf);
    //校验码
    sendbuf[34] = CSCheck(sendbuf,34);
    //结束码
    sendbuf[27] = 0x16;
    this->Send(sendbuf,36);
}

//测试卡命令
void CDL645::YK3()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x18;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XA2;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X05;
    //测试电量4字节

    //命令验证码算法YZ2
    sendbuf[27] = GetYZ2(sendbuf);
    //校验码
    sendbuf[28] = CSCheck(sendbuf,28);
    //结束码
    sendbuf[29] = 0x16;
    this->Send(sendbuf,30);
}

//查询卡命令
void CDL645::YK4()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 1, m_addrarea, sizeof(m_addrarea));

    sendbuf[7]	= 0x68;
    //控制码
    sendbuf[8]	= 0x0D;
    //数据长度
    sendbuf[9]	= 0x0E;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[20] = 0XA3;
    //命令数据域+命令验证码的长度
    sendbuf[21] = 0X02;
    //数据数据域
    sendbuf[22] = 0X00;
    //命令验证码算法YZ2
    sendbuf[23] = (ST_BYTE)GetYZ2(sendbuf);

    //數據域+0x33
    for(int i=0; i<14; i++)
    {
        sendbuf[10+i] += 0x33;
    }

    //校验码
    sendbuf[24] = CSCheck(sendbuf,24);
    //结束码
    sendbuf[25] = 0x16;
    this->Send(sendbuf,26);
}

//预置卡命令
void CDL645::YK7()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x14;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XA7;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X08;
    //数据格式D，7字节

    //命令验证码算法YZ2
    sendbuf[29] = GetYZ2(sendbuf);
    //校验码
    sendbuf[30] = CSCheck(sendbuf,30);
    //结束码
    sendbuf[31] = 0x16;
    this->Send(sendbuf,32);
}

//管理卡命令
void CDL645::YK8()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x17;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XA8;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X0B;
    //

    //命令验证码算法YZ2
    sendbuf[32] = GetYZ2(sendbuf);
    //校验码
    sendbuf[33] = CSCheck(sendbuf,33);
    //结束码
    sendbuf[34] = 0x16;
    this->Send(sendbuf,35);
}

//开户卡命令
void CDL645::YK9()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x45;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XA9;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X39;
    //数据格式A，56字节
    sendbuf[23] = 0x00;
    sendbuf[24] = 0x00;
    sendbuf[25] = 0x00;
    memcpy (sendbuf + 26, m_addrarea, sizeof(m_addrarea));
    //用户编号(6字节)
}

//购电卡命令
void CDL645::YK10(int charge)
{
    m_pLogger->LogDebug("query before charge!");
    m_pLogger->LogDebug("chargeCount = %d, totalCharge = %d",chargeArray[this->GetDeviceId()],chargeArray[this->GetDeviceId()]);
    int count = chargeArray[this->GetDeviceId()].chargeCount + 1;    //update charge count
    int totalCharge = charge + chargeArray[this->GetDeviceId()].totalChargeValue;     //update total chargeQuantity
    m_pLogger->LogDebug("YK10 started!");
    m_pLogger->LogDebug("chargeQuantity = %d, chargeCount = %d, totalCharge = %d",charge,count,totalCharge);
    ST_BYTE sendbuf[256] = {0};
    //起始码

    sendbuf[0]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 1, m_addrarea, sizeof(m_addrarea));

    sendbuf[7]	= 0x68;
    //控制码
    sendbuf[8]	= 0x0D;
    //数据长度
    sendbuf[9]	= 0x45;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[20] = 0XAA;
    //命令数据域+命令验证码的长度
    sendbuf[21] = 0X39;
    //数据格式A，56字节
    sendbuf[22] = 0x00;
    sendbuf[23] = 0x00;
    sendbuf[24] = 0x00;

    //通訊地址
    sendbuf[25] = m_addrarea[0];
    sendbuf[26] = m_addrarea[1];
    sendbuf[27] = m_addrarea[2];
    sendbuf[28] = m_addrarea[3];
    sendbuf[29] = m_addrarea[4];
    sendbuf[30] = m_addrarea[5];
    //用戶編號 45 C3 AB 89 67 45
    sendbuf[31] = 0x12;
    sendbuf[32] = 0x90;
    sendbuf[33] = 0x78;
    sendbuf[34] = 0x56;
    sendbuf[35] = 0x34;
    sendbuf[36] = 0x12;
    //系統密碼
    sendbuf[37] = 0x56;
    sendbuf[38] = 0x34;
    sendbuf[39] = 0x12;
    //隨機參數      A9 3E 33
    sendbuf[40] = 0x76;
    sendbuf[41] = 0x0B;
    sendbuf[42] = 0x00;
    //購電日期      73 4A 36 39 56
    sendbuf[43] = 0x40;
    sendbuf[44] = 0x17;
    sendbuf[45] = 0x03;
    sendbuf[46] = 0x06;
    sendbuf[47] = 0x23;
    //購電次數
    ToBCD(count,sendbuf+48,2);
    //本次購電電量
    ToBCD(charge*100,sendbuf+50,4);
    //總購電量
    ToBCD(totalCharge*100,sendbuf+54,4);
    //聲光報警電量
    ToBCD(chargeArray[this->GetDeviceId()].acousticQuantity*100,sendbuf+58,4);
    //拉渣報警電量
    ToBCD(chargeArray[this->GetDeviceId()].switchQuantity*100,sendbuf+62,4);
    //囤積限量
    ToBCD(chargeArray[this->GetDeviceId()].storage*100,sendbuf+66,4);
    //賒欠限量
    ToBCD(chargeArray[this->GetDeviceId()].credit*100,sendbuf+70,4);
    //縣容功率
    ToBCD(chargeArray[this->GetDeviceId()].limitPower*100,sendbuf+74,3);
    //縣容方式
    sendbuf[77] = 0x00;
    //命令验证码算法YZ2
    sendbuf[78] = (ST_BYTE)GetYZ2(sendbuf);


    //數據域+0x33
    for(int i=10; i<79; i++)
    {
        sendbuf[i] += 0x33;
    }

    //校验码
    sendbuf[79] = CSCheck(sendbuf,79);
    //结束码
    sendbuf[80] = 0x16;
    this->Send(sendbuf,81);

}

//退费卡命令
void CDL645::YK11()
{
    ST_BYTE sendbuf[256] = {0};
    //起始码
    sendbuf[0] = 0XFE;
    sendbuf[1]	= 0x68;
    //通讯地址(6字节)
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    m_addrarea[0] = (info.Addressex[10]-'0')*16 + (info.Addressex[11]-'0');
    m_addrarea[1] = (info.Addressex[ 8]-'0')*16 + (info.Addressex[ 9]-'0');
    m_addrarea[2] = (info.Addressex[ 6]-'0')*16 + (info.Addressex[ 7]-'0');
    m_addrarea[3] = (info.Addressex[ 4]-'0')*16 + (info.Addressex[ 5]-'0');
    m_addrarea[4] = (info.Addressex[ 2]-'0')*16 + (info.Addressex[ 3]-'0');
    m_addrarea[5] = (info.Addressex[ 0]-'0')*16 + (info.Addressex[ 1]-'0');

    memcpy (sendbuf + 2, m_addrarea, sizeof(m_addrarea));

    sendbuf[8]	= 0x68;
    //控制码
    sendbuf[9]	= 0x0D;
    //数据长度
    sendbuf[10]	= 0x45;
    //10字节验证加密
    GetRand(sendbuf);
    sendbuf[21] = 0XAB;
    //命令数据域+命令验证码的长度
    sendbuf[22] = 0X39;
    //数据格式A，56字节
}





ST_BYTE CDL645::DecToBCD(ST_BYTE num)
{
    ST_BYTE a, b, bcd;

    a = (num % 10) & 0x0f;
    b = ((num / 10) << 4) & 0xf0;
    bcd = a | b;

    return bcd;
}


ST_BYTE CDL645::CSCheck(ST_BYTE *buf,int len)
{
    ST_BYTE result = 0;
    for (int i=0; i<len; i++)
    {
        result += buf[i];
    }
    return result;
}














