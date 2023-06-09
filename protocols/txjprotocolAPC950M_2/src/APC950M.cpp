#include "APC950M.h"
#include "Channel.h"
 #include <stdio.h>

APC950M::APC950M()
{

    time(&Newcurtime);
    time(&oldcurtime);
    /*Newcurtime= clock();
	oldcurtime= clock();*/
    //ctor
}

APC950M::~APC950M()
{
    //dtor
}
APC950M* CreateInstace()
{
    return new APC950M();
}



ST_BOOLEAN	APC950M::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

//4F 4B 0D 0A 47 20 31 20 30 30 30 30 31 30 30 31 20 30 30 20 30 30 20 30 30 20 30 30 20 31 33 20 39 36 0D 0A 45 4E 44 0D 0A
void	APC950M::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    if(this->GetCurPort())
	{
		ST_INT len = this->GetCurPort()->PickBytes(pbuf, 12, 3000);
		if(len < 4) {
			//ShowMessage ("Insufficient data length");
			return;
		}
		if(pbuf[0]=='O'&&pbuf[1]=='K')
        {
            int ndatalen ;

            if(len<12)
            {
                ndatalen = this->GetCurPort()->ReadBytes(pbuf, 12);
                readed = ndatalen;
                return;
            }

            int dl = 9+(32*50);//读取设备数量为50一次
            ndatalen = this->GetCurPort()->ReadBytes(pbuf, dl);
            if(ndatalen>=40){
                readed = ndatalen;//ndatalen;
                return;
            }
		}
		else if(pbuf[0]=='A'&&pbuf[1]=='N'&&pbuf[2]=='S')
        {
            int ndatalen ;
            int dl = 23;
            ndatalen = this->GetCurPort()->ReadBytes(pbuf, dl);
            if(ndatalen>=23){
                readed = ndatalen;//ndatalen;
                return;
            }
        }
		else if((pbuf[0]=='F'&&pbuf[1]=='R'&&pbuf[2]=='E'&&pbuf[3]=='E')|     //FREE
                (pbuf[0]=='B'&&pbuf[1]=='U'&&pbuf[2]=='S'&&pbuf[3]=='Y')      //BUSY
                )
        {
            int ndatalen ;
            if(len<12)
            {
                ndatalen = this->GetCurPort()->ReadBytes(pbuf, 12);
                readed = ndatalen;
                return;
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
    return;
}
/*        char msg[64];
        sprintf(msg,"devcount :%d ||m_sendCount :%d\ninfo.Addressex[2]:%s\n",devcount,m_sendCount,info.Addressex[2]);
        ShowMessage (msg);*/

ST_BOOLEAN	APC950M::OnSend()
{
    m_task = false;

    if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
		    if(m_net_status == FREE_STATUS)
            {
                if(m_curTask.taskCmdCode == 1)
                {
                    ShowMessage("Enter YK  group. ");
                    ykAddr = m_curTask.taskAddr;
                    SendYK(m_curTask.taskAddr,m_curTask.taskValue);
                }

            }

		}
		m_task = true;
		return true;
	}

	if(! m_isBatch)
    {
        SendBATCH();
        //oldcurtime = clock();
        time(&oldcurtime);
        const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();  //读取扩展地址中的设备数量
        int devcount = 0;

        if(info.Addressex[2]!=NULL)
        {
            devcount = (info.Addressex[0]-0x30)*100+
                       (info.Addressex[1]-0x30)*10+
                       (info.Addressex[2]-0x30);
        }
        else
        {
            devcount = (info.Addressex[0]-0x30)*10+
                       (info.Addressex[1]-0x30);
        }
        m_sendCount = devcount/10;
        if((devcount%10)>0)
            m_sendCount++;   //如果余数大于0 则增加一次发送

        step = FSTATUS;
        return 1;
    }

    switch(step){
    case FBATCH:
        {
            //Newcurtime = clock();
            time(&Newcurtime);
            //if((abs(Newcurtime-oldcurtime) > 300 * CLOCKS_PER_SEC))
            if(difftime(Newcurtime,oldcurtime)>3600*12) //3600*12 12hour行一次抄表 60*60*1
            {
                //oldcurtime = clock();
                time(&oldcurtime);
                SendBATCH();
                step = FSTATUS;
            }
            else
            {
                SendSTATUS();
            }
        }
        break;
    case FRDNODE:
        {
            SendNDNODE(m_readidex);
            m_readidex ++;
            if(m_readidex == m_sendCount)
            {
                step = FBATCH;
                m_readidex = 0;
            }
        }break;
    case FSTATUS:
        {
            SendSTATUS();
        }break;
    case FCMD:
        {
            SendCMD(ykAddr);
            m_ykcmd = false;
            step = FSTATUS;
        }break;
    case FRST:
        {
            ShowMessage("Rst thread");
        }break;
    default:
        break;
    }


    return 1;
}
/*
4F 4B 0D 0A
47
20
31
20
30 30 30 30 31 30 30 31
20
30 30 20 30 30 20 30 30 20 31 30
20
31 33 20 39 36
0D 0A
45 4E 44
0D 0A
 */
ST_BOOLEAN	APC950M::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    if(m_task)
    {
        if(pbuf[0]== 'O'&& pbuf[1]== 'K'){
            m_curTask.taskResult.resultCode = 0;
            m_curTask.isTransfer = 1;
            Transfer(&m_curTask);
			Memset(&m_curTask, 0, sizeof(m_curTask));
			//SendCMD(ykAddr);
			m_ykcmd = true;
			step = FSTATUS;
			return true;
        }
        return false;
    }
    if(len>=40)
    {
        analyzeBATCH(pbuf,len);
    }
    else if(pbuf[0]=='A'&&pbuf[1]=='N'&&pbuf[2]=='S')
    {
        analyzeCMD(pbuf,len);
    }
    else if((pbuf[0]=='F'&&pbuf[1]=='R'&&pbuf[2]=='E'&&pbuf[3]=='E')|     //FREE
            (pbuf[0]=='B'&&pbuf[1]=='U'&&pbuf[2]=='S'&&pbuf[3]=='Y') )     //BUSY
    {
         if(pbuf[0]=='F')
         {
             if(m_ykcmd)
                step = FCMD;
             else
                step = FRDNODE;
             m_net_status = FREE_STATUS;
             ShowMessage("Net Status FREE");
         }
         if(pbuf[0]=='B')
         {
             step = FSTATUS;
             m_net_status = BUSY_STATUS;
             ShowMessage("Net Status BUSY");
         }
    }
    else
    {
        if(!m_isBatch)
                m_isBatch = !m_isBatch;
    }
    return 1;
}



 void  APC950M::analyzeBATCH(ST_BYTE* pbuf,ST_INT len)
 {
    int devcount = (len-9)/32;
    ShowMessage("analyzer date.");
    const DeviceInfo* info = this->GetDevice()->GetDeviceInfo();
    for(int i=0;i<devcount;i++)
    {
        int varIndex = ((pbuf[13+32*i]-0x30)*100)+
                        ((pbuf[14+32*i]-0x30)*10)+
                        ((pbuf[15+32*i]-0x30)*1)-1;//减一是因为内存变量从0开始


        ST_BYTE connSigle= (pbuf[4+32*i]==0x47)?0x01:0x00;
        this->UpdateValue(varIndex , connSigle);

        if(((pbuf[17+32*i])=='F' )&&
            ((pbuf[18+32*i])=='F' )&&
            ((pbuf[20+32*i])=='F' )&&
            ((pbuf[21+32*i])=='F' ))
        {
            ShowMessage("date unusual.");
        }
        else
        {

/*            ST_UINT32 value32 = 0;
            value32 = (value32|(pbuf[17+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[18+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[20+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[21+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[23+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[24+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[26+32*i]-0x30))<<4;
            value32 = (value32|(pbuf[27+32*i]-0x30));*/
            ST_UINT32 value32 = 0;
            value32 = (value32|switchValue(pbuf[17+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[18+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[20+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[21+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[23+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[24+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[26+32*i]))<<4;
            value32 = (value32|switchValue(pbuf[27+32*i]));
            float fv = (float)value32 * 0.1;
            /*const ST_DataAreaItem& itemref = this->GetDevice()->GetDeviceInfo()->DataAreas[1].items[varIndex-1];
            if(itemref.coeficient != 0)
            {
                fv = fv * itemref.coeficient;
            }
*/
            this->UpdateValue(varIndex+10000 , fv);  //old:1000 new:10000


            ST_BYTE openSigle = (pbuf[30+32*i]-0x30);
            ST_BYTE bvalue = (openSigle&0x03)==0x03?0:1;

            /*char msg[64];
            sprintf(msg,"bvalue :%d\n",bvalue);
            ShowMessage (msg);*/

            this->UpdateValue(varIndex+20000 , bvalue);//old:2000 new:20000
        }

    }
 }

ST_BYTE APC950M::switchValue(ST_BYTE bvalue)
{
    if((bvalue>='A')&&(bvalue<='F'))
        return (bvalue-0x37);
    if((bvalue >= '1')&&(bvalue<='9'))
        return (bvalue-0x30);
    return 0;
}


void  APC950M::analyzeCMD(ST_BYTE* pbuf,ST_INT len)
{
    int varIndex = ykAddr-1;
 /*   double dvalue = ((pbuf[4]-0x30)*10000000)+
                        ((pbuf[5]-0x30)*1000000)+
                        ((pbuf[7]-0x30)*100000)+
                        ((pbuf[8]-0x30)*10000)+
                        ((pbuf[10]-0x30)*1000)+
                        ((pbuf[11]-0x30)*100)+
                        ((pbuf[13]-0x30)*10)+
                        ((pbuf[14]-0x30)*1);*/
    ST_UINT32 value32 = 0;
    value32 = (value32|switchValue(pbuf[4]))<<4;
    value32 = (value32|switchValue(pbuf[5]))<<4;
    value32 = (value32|switchValue(pbuf[7]))<<4;
    value32 = (value32|switchValue(pbuf[8]))<<4;
    value32 = (value32|switchValue(pbuf[10]))<<4;
    value32 = (value32|switchValue(pbuf[11]))<<4;
    value32 = (value32|switchValue(pbuf[13]))<<4;
    value32 = (value32|switchValue(pbuf[14]));

    float fv = (float)value32*0.1;
    this->UpdateValue(varIndex+1000 , fv);

    ST_BYTE openSigle = (pbuf[17]-0x30);
    ST_BYTE bvalue = (openSigle&0x03)==0x03?0:1;

    char msg[64];
    sprintf(msg,"bvalue :%d\n",bvalue);
    ShowMessage (msg);

    this->UpdateValue(varIndex+20000 , bvalue);
}

void  APC950M::SendBATCH()
{
    ShowMessage("Send BATCH!");
    ST_BYTE sendbuf[64];
    sendbuf[0] = 'B';
    sendbuf[1] = 'A';
    sendbuf[2] = 'T';
    sendbuf[3] = 'C';
    sendbuf[4] = 'H';
    sendbuf[5] = 0x0D;
    sendbuf[6] = 0x0A;
/*    sendbuf[7] = 0x0D;
    sendbuf[8] = 0x0A;*/
    this->Send(sendbuf,7);

}
//52 44 4E 4F 44 45 20 30 20 31 30 0D 0A   RDNODE 0 10
void  APC950M::SendNDNODE()
{
    const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();
    ST_BYTE sendbuf[64];
    sendbuf[0] =  'R';
    sendbuf[1] =  'D';
    sendbuf[2] =  'N';
    sendbuf[3] =  'O';
    sendbuf[4] =  'D';
    sendbuf[5] =  'E';
    sendbuf[6] =  ' ';
    sendbuf[7] =  '0';
    sendbuf[8] =  ' ';
    sendbuf[9] =  '1';
    sendbuf[10] = '0';
    sendbuf[11] = 0x0D;
    sendbuf[12] = 0x0A;
 /*   sendbuf[13] = 0x0D;
    sendbuf[14] = 0x0A;*/
    this->Send(sendbuf,13);

}

void  APC950M::SendNDNODE(int ID)
{
    if(ID == 0){
        SendNDNODE();
        return ;
    }
    else{
        ST_BYTE sendbuf[64];
        sendbuf[0] = 'R';
        sendbuf[1] = 'D';
        sendbuf[2] = 'N';
        sendbuf[3] = 'O';
        sendbuf[4] = 'D';
        sendbuf[5] = 'E';
        sendbuf[6] = ' ';
        sendbuf[7] = '0';
        sendbuf[8] = '0';
        sendbuf[9] = '0';
        sendbuf[10] = '0';
        sendbuf[11] = '0';
        int rid = (10*ID)+1;
        sendbuf[12] = (rid / 100 % 10)+0x30;
        sendbuf[13] = (rid / 10 % 10)+0x30;
        sendbuf[14] = (rid / 1 % 10)+0x30;
        sendbuf[15] = ' ';
        sendbuf[16] = '1';
        sendbuf[17] = '0';
        sendbuf[18] = 0x0D;
        sendbuf[19] = 0x0A;
        this->Send(sendbuf,20);
    }

}

void  APC950M::SendCMD(ST_UINT writeAddr)
{
    ShowMessage("Send CMD!");
    ST_BYTE sendbuf[64];
    sendbuf[0] = 'C';
    sendbuf[1] = 'M';
    sendbuf[2] = 'D';
    sendbuf[3] = 0x20;
    sendbuf[4] = '0';
    sendbuf[5] = '0';
    sendbuf[6] = '0';
    sendbuf[7] = '0';
    sendbuf[8] = '0';
    sendbuf[9] = (writeAddr / 100 % 10)+0x30;
    sendbuf[10] = (writeAddr / 10 % 10)+0x30;
    sendbuf[11] = (writeAddr / 1 % 10)+0x30;
    sendbuf[12] = 0x0D;
    sendbuf[13] = 0x0A;
    this->Send(sendbuf,14);
}


void  APC950M::SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
    ST_BYTE sendbuf[64];
    sendbuf[0] = 'T';
    sendbuf[1] = 'S';
    sendbuf[2] = 'T';
    sendbuf[3] = 0x20;
    sendbuf[4] = '0';
    sendbuf[5] = '0';
    sendbuf[6] = '0';
    sendbuf[7] = '0';
    sendbuf[8] = '0';
    sendbuf[9] = (writeAddr / 100 % 10)+0x30;
    sendbuf[10] = (writeAddr / 10 % 10)+0x30;
    sendbuf[11] = (writeAddr / 1 % 10)+0x30;
    sendbuf[12] = 0x20;
    sendbuf[13] = '1';
    sendbuf[14] = '9';
    sendbuf[15] = bIsOn==1?'4':'5';
    sendbuf[16] = 0x0D;
    sendbuf[17] = 0x0A;
    this->Send(sendbuf,18);
}

void  APC950M::SendSTATUS()
{
    ST_BYTE sendbuf[64];
    sendbuf[0] = 'S';
    sendbuf[1] = 'T';
    sendbuf[2] = 'A';
    sendbuf[3] = 'T';
    sendbuf[4] = 'U';
    sendbuf[5] = 'S';
    sendbuf[6] = 0x0D;
    sendbuf[7] = 0x0A;
    this->Send(sendbuf,8);
}
