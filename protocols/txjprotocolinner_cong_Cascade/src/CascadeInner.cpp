#include "CascadeInner.h"
#include <unistd.h>
#include "EngineBase.h"
#include <math.h>
#include "Channel.h"
#include "Communication.h"
#include "Debug.h"
#include "syslogger.h"
#include "Debug.h"

#define wDebug		if(false)wedoDebug(SysLogger::GetInstance()).noquote

extern NodeTree *g_pTree;



CascadeInner::CascadeInner()
    :is_initial(true)
{
    //ctor
}

CascadeInner::~CascadeInner()
{
    //dtor
}

void    CascadeInner::Init()
{
    is_init = InitDevInfo();
    if(!is_init)
    {
        m_pLogger->GetInstance()->LogError("not found device ID : %d",this->GetDevice()->GetDeviceInfo()->Address);
    }

    nsendfailnum = 0;
    for(ST_INT ib = 0; ib < 1024; ++ib)
    {
        m_Begin[ib] = true;
    }
    m_bTask      = false;
    is_totalcall = false;
}

void	CascadeInner::Uninit()
{

}

void CascadeInner::InitTransfertable()
{
    m_pLogger->LogDebug("Enter Init transfer\n");
    printf("Enter Init transfer\n");

    for (int index=0; index<3; index++)
    {
        m_pLogger->LogDebug("Init transfer %d time\n",index);
        TransferTable *trantable   = NULL;
        List<ST_DUADDR> *tablelist = NULL;

        if (!CheckTransferTableExist (index, trantable, tablelist))
            continue;

        int32_t list_size = tablelist->GetCount();
        if (list_size <= 0)
        {
            list_size = 0;
        }

        for (int32_t iter = 0; iter < list_size; ++iter)
        {
            ST_DUADDR  *duaddr = tablelist->GetItem(iter);

            //綁定設備名和點名
            Device *t_dev = this->m_pChannel->GetCommunication()->GetChannel(duaddr->connect)->GetDevice(duaddr->device);
            string s_devName = t_dev->GetDeviceInfo()->DeviceName;

            ST_CHAR  ditName[64];
            GetDitNameByAddr(t_dev->GetDeviceInfo(),duaddr->addr,ditName);
            ST_CHAR fullName[256] = {0};

            replace(s_devName.begin(),s_devName.end(),'-','.');
            strcpy(fullName, s_devName.c_str());
            strcat(fullName, ".");
            strcat(fullName, ditName);


            //封裝varNode結構體
            struct varNode var;
            var.duaddr = duaddr;
            var.nodeName = fullName;
            //將var元素插入vector
            vec_Node.push_back(var);
        }
    }
    m_pLogger->LogDebug("Init transfer completed!\n");
}


bool CascadeInner::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
    if (!this->GetDevice())
        return false;
    List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
    if ( !trantables)
    {
        printf("No transfer tables.\n");
        m_pLogger->LogDebug("No transfer tables.\n");
        return false;
    }

    if (trantables->GetCount() <= 0)
    {
        printf("Transfer table count is 0.\n");
        m_pLogger->LogDebug("Transfer table count is 0.\n");
        return false;
    }
    int iter = 0;
    for (; iter < trantables->GetCount(); ++iter)
    {
        if (index == trantables->GetItem(iter)->typeId())
            break;
    }
    if ((table = trantables->GetItem(iter)) == NULL)
    {
        m_pLogger->LogDebug("Not have this transfer table.index:%d\n",index);
        printf("Not have this transfer table.index:%d\n",index);
        return false;
    }
    if ((list = table->m_pVarAddrs) == NULL)
    {
        printf("Not have this list.\n");
        m_pLogger->LogDebug("Not have this list.\n");
        return false;
    }
    return true;
}


int CascadeInner::InitDevInfo()
{
    int devId = this->GetDevice()->GetDeviceInfo()->Address;
    int count = this->m_pChannel->GetCommunication()->GetChannels()->GetCount();
    for(int i=0; i<count; i++)
    {
        // 先遍历通道，再遍历通道下的设备，最后再比对设备地址是否一致，一致则获取其设备名，为之后获取设备下的数据做准备
        if(this->m_pChannel->GetCommunication()->GetChannels()->GetItem(i)->GetDevices()!=NULL)
        {
            //printf("channle name is :%s\r\n",this->m_pChannel->GetCommunication()->GetChannels()->GetItem(i)->GetChannelInfo()->ChannelName);
            Device *ptr_dev = this->m_pChannel->GetCommunication()->GetChannels()->GetItem(i)->GetDevice(devId);
            if(ptr_dev != NULL)
            {
                m_devName = ptr_dev->GetDeviceInfo()->DeviceName;
                return 1;
            }
        }
    }
    return 0;
}


void        CascadeInner::GetDitNameByAddr(DeviceInfo *devinfo,int id,ST_CHAR* ditName)
{
    int areaCount = devinfo->DataAreasCount;
    for(int i=0; i<areaCount; i++)
    {
        int itemsize = devinfo->DataAreas[i].itemCount;
        for(int j=0; i<itemsize; j++)
        {
            int addr = devinfo->DataAreas[i].items[j].id;
            if(addr==id)
            {
                Strcpy(ditName,devinfo->DataAreas[i].items[j].itemName);
                return ;
            }
        }
    }
}



void	CascadeInner::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    if(this->HasTask() && this->GetTask(&m_curTask))
    {
        if     (!strcmp(m_curTask.taskCmd,"SOE"))
        {
            SendSOE(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
        {
            SendYKEcho(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"singlewrite"))
        {
            SendYTEcho(m_curTask);
        }
        else if(!strcmp(m_curTask.taskCmd,"user_by_binary"))
        {
            ST_BYTE buf[32] = {0};
            buf[0] = 0xaa;
            buf[1] = 0xbb;
            buf[2] = 0x40;
            buf[3] = 0x03;
            buf[4] = 0x00;
            buf[5] =  m_curTask.transDeviceId&0x00ff;   //deviceid
            buf[6] = (m_curTask.transDeviceId&0xff00)>>8;
            buf[7] =  m_curTask.taskResult.resultCode;
            this->Send (&buf[0], 8);
            Memset(&m_curTask, 0, sizeof(m_curTask));
        }
        else if(!strcmp(m_curTask.taskCmd,"multiwrite"))
        {
            SendYKEcho(m_curTask);
        }
    }

    readed = 0;
    if(this->GetCurPort())
    {
        ST_INT	len = this->GetCurPort()->PickBytes(pbuf, 5, 300);
        if (len >= 5)
        {
            ST_INT star = 0;
            for(star = 0; star < len; star++)
            {
                if(pbuf[star] == 0xaa)
                    break;
            }
            if(star > 0)
            {
                this->GetCurPort()->ReadBytes(pbuf,star);
                return;
            }
            if((pbuf[0] == 0xaa) && (pbuf[1] == 0xbb))
            {
                int ndatalen = pbuf[3] + pbuf[4] * 256 + 5;
                len = this->GetCurPort()->PickBytes(pbuf, ndatalen, 1000);
                if(len >= ndatalen)
                {
                    len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
                    readed = len;
                }
                else
                {
                    this->GetCurPort()->Clear();
                }
            }
            else
            {
                this->GetCurPort()->Clear();
            }
        }
        else
        {
            this->GetCurPort()->Clear();
        }
    }
    return;
}

ST_BOOLEAN	CascadeInner::OnSend()
{
//	m_bTask = false;
    if(!this->IsOpened())
    {
        for(ST_INT ib = 0; ib < 1024; ib++)
        {
            m_Begin[ib] = true;
        }
        return false;
    }
    else if (is_initial)
    {
        is_initial = false;
        Thread::SLEEP(1000);
        ST_BYTE data[] = { 0xAA, 0xBB, 0x2B, 0x00, 0x00 };
        this->Send(data, sizeof(data));
        return true;
    }
    InitTransfertable();
    SendData();
    return true;
}

ST_BOOLEAN	CascadeInner::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    if(len < 5)
        return false;

    switch(pbuf[2])
    {
    case 0x07:
    {
        ST_BYTE data[] = {0xAA, 0xBB, 0x0B, 0x00, 0x00};
        this->Send (data, sizeof(data));
        Thread::SLEEP (20);
    }
    break;
    case 0x10:
    {
        switch (pbuf[7])
        {
        case 0x01:   // YK
        {
            ProtocolTask task;
            task.isTransfer     = true;
            task.transChannelId = -1;
            task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
            Strcpy(task.taskCmd, "devicecontrol");
            task.taskCmdCode = pbuf[8];
            task.taskAddr    = pbuf[9] + pbuf[10] * 256;
            task.taskValue   = pbuf[11];
            Transfer(&task);
        }
        break;
        case 0x02:   // YT
        {
            ProtocolTask task;
            task.isTransfer     = true;
            task.transChannelId = -1;
            task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
            strcpy(task.taskCmd, "singlewrite");
            task.taskCmdCode = pbuf[8];
            task.taskAddr    = pbuf[9] + pbuf[10] * 256;
            float value = 0;
            memcpy(&value, pbuf + 11, sizeof(value));
            task.taskValue = value;
            Transfer(&task);
        }
        break;
        }
    }
    break;
    case 0x20:
    {
        size_t plen = pbuf[3] + pbuf[4] * 256 - 2;
        if (plen <= 0)
            break;

        ProtocolTask task;
        task.isTransfer     = true;
        task.transChannelId = -1;
        task.transDeviceId  = pbuf[5] + pbuf[6] * 256;
        Strcpy(task.taskCmd, "user_by_binary");
        task.taskParamLen = plen;
        memcpy(task.taskParam, pbuf + 7, plen);
        Transfer(&task);
    }
    break;
    case 0x43:
    {
        ST_BYTE data[] = {0xAA, 0xBB, 0x83, 0x00, 0x00};
        this->Send (data, sizeof(data));
        Thread::SLEEP (20);
    }
    break;
    case 0x83:
    {

    } break;
    case 0xFF:
    {
        ST_BYTE data[] = {0xAA, 0xBB, 0xFF, 0x00, 0x00};
        this->Send (data, sizeof(data));
        Thread::SLEEP (20);
        is_totalcall = true;
    }
    break;
    default:
        break;
    }
    return true;
}

ST_BOOLEAN	CascadeInner::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

CascadeInner* CreateInstace()
{
    return new  CascadeInner();
}

void  CascadeInner::SendSOE(ProtocolTask& curTask)
{
//	if(curTask.taskCmdCode == 0)
    {
        ST_BYTE data[64] = {0};
        data[0] = 0xaa;
        data[1] = 0xbb;
        data[2] = 0x02;
        data[3] = 0x0e;
        data[4] = 0x00;
        data[5] = curTask.taskParam[12];   //deviceid
        data[6] = curTask.taskParam[13];
        data[7] = curTask.taskParam[10];     //point
        data[8] = curTask.taskParam[11];
        data[9] = curTask.taskParam[9];//statu
        data[10] = curTask.taskParam[0]; //year_L
        data[11] = curTask.taskParam[1]; //year_H
        data[12] = curTask.taskParam[2]; //month
        data[13] = curTask.taskParam[3]; //day
        data[14] = curTask.taskParam[4]; //hour
        data[15] = curTask.taskParam[5]; //minute
        data[16] = curTask.taskParam[6]; //second
        data[17] = curTask.taskParam[7]; //msecond_L
        data[18] = curTask.taskParam[8]; //msecond_H
        this->Send(data, 19);
        Memset(&curTask, 0, sizeof(curTask));
    }
}

void    CascadeInner::SendYKEcho(ProtocolTask& curTask)
{
    if((curTask.taskCmdCode == 0) || (curTask.taskCmdCode == 1) || (curTask.taskCmdCode == 2))
    {
        ST_BYTE bySendbuffer[64];
        bySendbuffer[0] = 0xaa;
        bySendbuffer[1] = 0xbb;
        bySendbuffer[2] = 0x80;
        bySendbuffer[3] = 0x03;
        bySendbuffer[4] = 0x00;
        bySendbuffer[5] = curTask.transDeviceId&0xff;   //deviceid
        bySendbuffer[6] = (curTask.transDeviceId&0xff00)>>8;
        bySendbuffer[7] = curTask.taskResult.resultCode;
//        m_pLogger->LogDebug("curTask.taskResult.resultCode%d",curTask.taskResult.resultCode);
        this->Send(&bySendbuffer[0],8);
        Memset(&curTask, 0, sizeof(curTask));
    }
}

void CascadeInner::SendYTEcho (ProtocolTask & _task)
{
    ST_BYTE data[32] = { 0xAA, 0xBB, 0x00 };
    data[2] = 0x80;
    data[3] = 0x03;
    data[4] = 0x00;
    data[5] = _task.transDeviceId % 256;
    data[6] = _task.transDeviceId / 256;
    data[7] = _task.taskResult.resultCode;

    this->Send(&data[0], 8);
    Memset(&_task, 0, sizeof(_task));
}

void CascadeInner::SendData()
{
    //1.檢查轉發表
    if(vec_Node.size()==0)
    {
        ShowMessage("transfer table is null");
        return ;
    }

    time_t tt = time(0);
    //产生“YYYY-MM-DD hh:mm:ss”格式的字符串。
    char sTime[32];
    strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", localtime(&tt));


    ST_INT interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
    interval = interval > 30 ? interval : 30;

    ST_INT  nDataLen = 0;

    for(unsigned int i = 0; i < vec_Node.size(); ++i)
    {
        ST_VARIANT vValue;
        GetVariableValueByAddr(*vec_Node[i].duaddr, vValue);

        if(vec_Node[i].duaddr->type == -2)
            continue;
        if(vec_Node[i].duaddr->type == -1)
            continue;

        bySendbuf[0] = 0xaa;
        bySendbuf[1] = 0xbb;
        bySendbuf[2] = 0x01;
        bySendbuf[5] =  vec_Node[i].duaddr->connect& 0xff;                  //数据长度L
        bySendbuf[6] = (vec_Node[i].duaddr->connect& 0xff00)>>8;            //數據長度H
        bySendbuf[7] =  vec_Node[i].duaddr->device & 0xff;                  //通道地址L
        bySendbuf[8] = (vec_Node[i].duaddr->device & 0xff00)>>8;            //通道地址H
        bySendbuf[9+nDataLen] = vec_Node[i].duaddr->addr & 0xff;          //該設備第一個變化數據域地址L
        bySendbuf[10+nDataLen] = (vec_Node[i].duaddr->addr & 0xff00)>>8;    //該設備第一個變化數據域地址H
        bySendbuf[11+nDataLen] = vValue.vt;                 //第一個變化數據的數據類型
        bySendbuf[12+nDataLen] = vec_Node[i].duaddr->type;                  //第一個變化數據的設備變量
        if(vec_Node[i].duaddr->type == -1)
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
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.bVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.bVal);
        }
        break;
        case VALType_Byte:
        {
            ST_BYTE Value = vValue.bVal;
            bySendbuf[13+nDataLen] = Value;
            nDataLen = nDataLen+5;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.bVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.bVal);
        }
        break;
        case VALType_Boolean:
        {
            ST_BYTE Value = vValue.blVal;
            bySendbuf[13+nDataLen] = Value;
            nDataLen = nDataLen+5;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.blVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.bVal);
        }
        break;
        case VALType_Int16:
        {
            ST_INT Value  = vValue.sVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,2);
            nDataLen = nDataLen+6;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.sVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.sVal);
        }
        break;
        case VALType_UInt16:
        {
            ST_INT Value  = vValue.usVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,2);
            nDataLen = nDataLen+6;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.usVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.usVal);
        }
        break;
        case VALType_Int32:
        {
            ST_INT Value  = vValue.iVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,4);
            nDataLen = nDataLen+8;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.iVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.iVal);
        }
        break;
        case VALType_UInt32:
        {
            ST_INT Value  = vValue.uiVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,4);
            nDataLen = nDataLen+8;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.uiVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.uiVal);

        }
        break;
        case VALType_Float:
        {
            ST_FLOAT Value  = vValue.fVal;
            memcpy(&bySendbuf[13 + nDataLen], &Value, 4);
            nDataLen = nDataLen + 8;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.fVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %f",vec_Node[i].nodeName.c_str(),vValue.fVal);
        }
        break;
        case VALType_Int64:
        {
            ST_LONG Value  = vValue.lVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,8);
            nDataLen = nDataLen+12;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.lVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.lVal);
        }
        break;
        case VALType_UInt64:
        {
            ST_LONG Value  = vValue.ulVal;
            memcpy(&bySendbuf[13+nDataLen],&Value,8);
            nDataLen = nDataLen+12;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.ulVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %d",vec_Node[i].nodeName.c_str(),vValue.ulVal);
        }
        break;
        case VALType_Double:
        {
            ST_DOUBLE Value  = vValue.dtVal;
            memcpy(&bySendbuf[13 + nDataLen], &Value, 8);
            nDataLen = nDataLen + 12;
            //cout << "variable name = " << vec_Node[i].nodeName << "\tvalue = " << vValue.dtVal <<endl;
            m_pLogger->LogDebug("variable name = %s,value = %lf",vec_Node[i].nodeName.c_str(),vValue.dtVal);
        }
        break;
        default:
            break;
        }
        if (nDataLen >= 300)
        {
            nDataLen += 4;
            bySendbuf[3] =  nDataLen & 0xff;
            bySendbuf[4] = (nDataLen & 0xff00) >> 8;
            this->Send(bySendbuf, nDataLen + 5);
            Thread::SLEEP(interval);
            nDataLen = 0;
        }
    }
}
