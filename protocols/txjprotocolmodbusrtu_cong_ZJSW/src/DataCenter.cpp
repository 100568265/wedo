#include "DataCenter.h"
#include "Channel.h"
#include "tinyxml.h"
#include "syslogger.h"
#include <sys/time.h>
#include <stdlib.h>

extern NodeTree *g_pTree;

DataCenter::DataCenter()
{
    //ctor
}

DataCenter::~DataCenter()
{
    //dtor
}

void     DataCenter::getbefordayString(int bTime,char *dest)
{
    time_t now;
    struct tm  *ts;
    char yearchar[64];
    now = time(NULL);
    ts = localtime(&now);
    ts->tm_mday = ts->tm_mday-bTime;
    mktime(ts); /* Normalise ts */
    strftime(yearchar, sizeof(yearchar), "%Y%m%d", ts);
    strcpy(dest,yearchar);
}

void  DataCenter::delBeforFile()
{
    char befday[12] = {0},filename[64]={0};
    getbefordayString(7,befday);
    sprintf(filename,"storage/%s.xml",befday);

    remove(filename);
}

void  DataCenter::checkFileExist()
{
    char cToday[64] = {0},filename[64]={0};

    getbefordayString(0,cToday);
    sprintf(filename,"storage/%s.xml",cToday);

	TiXmlDocument pReadDocument ;
    if(!pReadDocument.LoadFile(filename))
    {
        SysLogger::GetInstance()->LogWarn ("DataCenter::checkFileExist() don't exist!");
        writeInitXml();    //打开失败，则创建文件
        delBeforFile();    //删除7天前的数据
    }
}

void  DataCenter::getlasttimeinsert(curTime *lastTime)
{

    char cToday[64] = {0},filename[64]={0};
    lastTime->dHour = -1;
    lastTime->dminute = -1;

    getbefordayString(0,cToday);
    sprintf(filename,"storage/%s.xml",cToday);

    TiXmlDocument pReadDocument;
    if(pReadDocument.LoadFile(filename)){
        // 得到文件根节点
        TiXmlElement* pRootElement = pReadDocument.RootElement();
        TiXmlElement* areaItem = pRootElement->FirstChildElement("Area");
        TiXmlElement* dateItem = (TiXmlElement*)areaItem->LastChild();
        if(dateItem!=NULL)
        {
            string dtime(dateItem->Attribute("time"));
            lastTime->dHour = atoi((dtime.substr(0,2)).c_str());
            lastTime->dminute = atoi((dtime.substr(3)).c_str());
        }

    }
    else
        checkFileExist();

}

void  DataCenter::writeInitXml()
{
    char cToday[64] = {0},filename[64]={0};

    getbefordayString(0,cToday);
    sprintf(filename,"storage/%s.xml",cToday);

    TiXmlDocument* pWriteDoc = new TiXmlDocument();
    TiXmlDeclaration *pDeclare = new TiXmlDeclaration("1.0","UTF-8","yes");
    pWriteDoc->LinkEndChild(pDeclare);

    TiXmlElement *pRootElement = new TiXmlElement("History");
    pRootElement->SetAttribute("Day",cToday);

    ////////////////////////////////////////////////////////////////////////
	ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
    for(ST_INT i = 0; i < devcount; ++i)
	{
		if (g_pTree->GetNodeName("", i, devname) < 0)
            continue;

        ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        if (varcount <= 0) continue;

        *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

        g_pTree->GetNodeName(devname, 0, ditname);
        if(strcmp(devname,"channelstate")==0|strcmp(devname,"devicestate")==0) //将设备状态和通道状态去除
            continue;

        strcpy(fullname, devname); strcat(fullname, ".");
        strcat(fullname, ditname); strcat(fullname, ".value");
        ST_DUADDR tdd;
        g_pTree->GetNodeAddr(fullname, tdd);

        TiXmlElement* areaElmt = new TiXmlElement("Area");
        char abuf[8];
        sprintf(abuf,"%d",tdd.device);
        areaElmt->SetAttribute("deviceId",abuf);
        areaElmt->SetAttribute("name",devname);
        pRootElement->LinkEndChild(areaElmt);
	}
    ///////////////////////////////////////////////////////////////////////

    pWriteDoc->LinkEndChild(pRootElement);
    pWriteDoc->SaveFile(filename);
    delete pRootElement;

}

void   DataCenter::inserAllDataInXml()
{
//    printf("DataCenter::inserAllDataInXml()\n");
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);   //Get time message

    curTime lastData;
    getlasttimeinsert(&lastData);
//    printf("lastdata->dHour:%d lastdata->dminute:%d\n",lastData.dHour,lastData.dminute);
    if(lastData.dHour!=-1&&lastData.dminute!=-1)
    {
         int historyMin = lastData.dHour*60+lastData.dminute;
         int currentMin = t->tm_hour*60+t->tm_min;

          if((currentMin-historyMin)>=5)
         {

             if(t->tm_hour>lastData.dHour||(currentMin-historyMin)>10)  //防止出现中途断线等情况
            {
                struct curTime cData;
                cData.dHour = t->tm_hour;
                int i=0;
                int cmin = t->tm_min;
                while(i<6)
                {
                    if(((cmin)%5)==0)
                        break;
                    i++;
                    cmin--;
                }
                cData.dminute = cmin;
                writeAllInXml(cData);
            }
            else
            {
                struct curTime cData;
                cData.dHour = t->tm_hour;
                if((lastData.dminute+5)==60)
                    cData.dminute = 0;
                else
                    cData.dminute = lastData.dminute+5;
                writeAllInXml(cData);
            }

         }
         else
            return;
    }
    else
    {
        printf("Enter fuction 2\n");
        if(t->tm_hour==0 && t->tm_min<5)
        {
            struct curTime cData;
            cData.dHour = 0;
            cData.dminute = 0;
            writeAllInXml(cData);
        }
        else
        {

            struct curTime cData;
            int i = 0;
            int cmin = t->tm_min;
            while(i<6)
            {
                if(((cmin)%5)==0)
                        break;
                i++;
                cmin--;
            }
            cData.dHour = t->tm_hour;
            cData.dminute = cmin;
            printf("Enter fuction 2-1\n");
            writeAllInXml(cData);
        }
    }
}

void     DataCenter::writeAllInXml(struct curTime tData)
{
    //printf("Enter writeAllInXml\n");
    char cToday[64] = {0},filename[64]={0};

    getbefordayString(0,cToday);
    sprintf(filename,"storage/%s.xml",cToday);

    TiXmlDocument pReadDocument;
    if(!pReadDocument.LoadFile(filename))
    {
        checkFileExist();
    }
    else
    {
        TiXmlElement *pRootElement = pReadDocument.RootElement();

        ////////////////////////////////////////////////////////////////////////
        ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
        ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
        for(ST_INT i = 0; i < devcount; ++i)
        {
            //printf("devcount foreach :%d",i);
            if (g_pTree->GetNodeName("", i, devname) < 0)
                continue;

            ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
            if (varcount <= 0) continue;

            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

            g_pTree->GetNodeName(devname, 0, ditname);
            if(strcmp(devname,"channelstate")==0|strcmp(devname,"devicestate")==0) //将设备状态和通道状态去除
                continue;

            strcpy(fullname, devname); strcat(fullname, ".");
            strcat(fullname, ditname); strcat(fullname, ".value");
            ST_DUADDR tdd;
            g_pTree->GetNodeAddr(fullname, tdd);

            for (TiXmlElement* areaItem = pRootElement->FirstChildElement("Area"); areaItem !=0; areaItem = areaItem->NextSiblingElement("Area"))
            {

                int aId = atoi(areaItem->Attribute("deviceId"));
                //printf("aId foreach :%d\n",aId);

                if(aId == tdd.device)
                {
                    TiXmlElement *dateElement = new TiXmlElement("Date");
                    char buffer[24];
                    sprintf(buffer,"%02d:%02d",tData.dHour,tData.dminute);//格式化时间
                    dateElement->SetAttribute("time",buffer);

                    for(int j = 0; j < varcount; ++j)
                    {
                        //printf("varcount foreach :%d\n",j);
                        *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

                        g_pTree->GetNodeName(devname, j, ditname);

                        strcpy(fullname, devname); strcat(fullname, ".");
                        strcat(fullname, ditname); strcat(fullname, ".value");

                        ST_DUADDR tdd;
                        g_pTree->GetNodeAddr(fullname, tdd);
                        ST_VARIANT vValue;
                        GetVariableValueByAddr(tdd, vValue);

                        if(tdd.type == -2) continue;
                        switch(vValue.vt)
                        {
                        case VALType_Float:
                        {
                            ST_FLOAT Value  = vValue.fVal;

                            TiXmlElement *dataElement = new TiXmlElement("Data");
                            dataElement->SetAttribute("id",tdd.addr);
                            dataElement->SetAttribute("name",ditname);

                            char cValue[24];//转换后的字符串
                            sprintf(cValue, "%.2f", Value);

                            TiXmlText *fText = new TiXmlText(cValue);
                            dataElement->LinkEndChild(fText);
                            dateElement->LinkEndChild(dataElement);
                        }
                            break;
                        case VALType_Double:
                        {
                            ST_DOUBLE Value  = vValue.dtVal;

                            TiXmlElement *dataElement = new TiXmlElement("Data");
                            dataElement->SetAttribute("id",tdd.addr);
                            dataElement->SetAttribute("name",ditname);

                            char cValue[24];//转换后的字符串
                            sprintf(cValue, "%.2lf", Value);

                            TiXmlText *fText = new TiXmlText(cValue);
                            dataElement->LinkEndChild(fText);
                            dateElement->LinkEndChild(dataElement);
                        }
                            break;
                        default:
                            break;
                        }
                    }
                    //break;
                    areaItem->LinkEndChild(dateElement);
                }

            }

        }
        pReadDocument.SaveFile(filename);
    }
}

int    DataCenter::getHistoryData(int befday, int dtime, ST_BYTE *pbuf)
{
    printf("befday : %d,dtime :%d\n",befday,dtime);

    char cToday[64] = {0},filename[64]={0};

    int dindex = 0;
    Memset(m_hisdlist,0,sizeof(m_hisdlist));

    int hour = (dtime * 5) / 60;
    int mins = (dtime * 5) % 60;
    printf("hour : %d,mins :%d\n",hour,mins);

    getbefordayString(befday-1,cToday);
    sprintf(filename,"storage/%s.xml",cToday);

    TiXmlDocument pReadDocument;
    if(!pReadDocument.LoadFile(filename))
    {
        return -1;
    }
    else
    {
        TiXmlElement *pRootElement = pReadDocument.RootElement();

        for (TiXmlElement* areaItem = pRootElement->FirstChildElement("Area"); areaItem !=0; areaItem = areaItem->NextSiblingElement("Area"))
        {
            for (TiXmlElement* dateItem = areaItem->FirstChildElement("Date"); dateItem!= 0; dateItem = dateItem->NextSiblingElement("Date"))
                {
                    //int vId = atoi(dataItem->Attribute("time"));
                    string dtime(dateItem->Attribute("time"));

                    int dHour = atoi((dtime.substr(0,2)).c_str());
                    int dminute = atoi((dtime.substr(3)).c_str());

                    if(hour == dHour && mins == dminute)
                    {
                        printf("get true time\n");
                       for (TiXmlElement* dataItem = dateItem->FirstChildElement("Data"); dataItem!= 0; dataItem = dataItem->NextSiblingElement("Data"))
                        {
                            int vid = atoi(dataItem->Attribute("id"));
                            double dvalue  = strtod(dataItem->GetText(),NULL);
                            m_hisdlist[dindex][vid] = dvalue;
                            printf("m_hisdlist value :%lf\n",m_hisdlist[dindex][vid]);
                        }
                        dindex++;
                        break;
                    }
                    //break;
                }
        }
    }
    if(dindex == 0)   //
        return -1;
    assemblydata(pbuf);
    return 1;
}

void     DataCenter::calculationValue(double d1,double d2,double d3,double d4,ST_BYTE *dataptr)
{
    double totoal  = d1+d2+d3+d4;
    dv2UIntt32(totoal,dataptr);
}

void    DataCenter::fv2UIntt16(float fvalue,ST_BYTE *dataptr)
{
    ST_UINT16 tmp16 = (ST_UINT16)fvalue;
    ST_UINT16 *transptr = (ST_UINT16*)&tmp16;
    *dataptr++  = (*transptr >> 0x08) & 0xFF;
    *dataptr++ = (*transptr >> 0x00) & 0xFF;
}


void    DataCenter::dv2UIntt32(double dvalue,ST_BYTE *dataptr)
{
    ST_UINT32 tmp16 = (ST_UINT32)dvalue;
    ST_UINT32     *transptr = (ST_UINT32*)&tmp16;
    *dataptr++ = (*transptr >> 0x18) & 0xFF;
    *dataptr++ = (*transptr >> 0x10) & 0xFF;
    *dataptr++ = (*transptr >> 0x08) & 0xFF;
    *dataptr++ = (*transptr >> 0x00) & 0xFF;
}

double      DataCenter::getdData2tableList(int i,int j)
{
    return m_hisdlist[i][j];
}
float       DataCenter::getfData2tableList(int i,int j)
{
    float tmpfvalue = (float)m_hisdlist[i][j];
    return tmpfvalue;
}

void    DataCenter::assemblydata(ST_BYTE *dbuf)
{
    printf("Enter DataCenter::assemblydata()\n ");
    dbuf[0] = 0x00;
    dbuf[1] = 0x00;

    calculationValue(getdData2tableList(0,8),getdData2tableList(1,8),getdData2tableList(2,8),getdData2tableList(3,8),&dbuf[2]);//2 3 4 5 总水表行度
    fv2UIntt16((getfData2tableList(0,7)+getfData2tableList(1,7)+getfData2tableList(2,7))+getfData2tableList(3,7),&dbuf[6]);//6 7         总水表瞬时流量

    //低区机泵状态
    //fv2UIntt16(getfData2tableList(0,10),&dbuf[8]);     //低区机泵状态
    fv2UIntt16(getfData2tableList(0,0),&dbuf[10]);    //低区1#机泵电流
    fv2UIntt16(getfData2tableList(0,1),&dbuf[12]);    //低区2#机泵电流
    fv2UIntt16(getfData2tableList(0,2),&dbuf[14]);    //低区3#机泵电流
    fv2UIntt16(getfData2tableList(0,3),&dbuf[16]);   //低区变频器频率
    fv2UIntt16(getfData2tableList(0,20),&dbuf[18]);   //低区设定压力
    fv2UIntt16(getfData2tableList(0,5),&dbuf[20]);   //低区出水压力
    dv2UIntt32(getdData2tableList(0,9),&dbuf[22]);   //低区电量行度  22 23 24 25
    dbuf[26] = 0x00;dbuf[27] = 0x00;dbuf[28] = 0x00;dbuf[29] = 0x00;  //备用

    //中区机泵状态
    //fv2UIntt16(getfData2tableList(1,10),&dbuf[30]);   //中区机泵状态
    fv2UIntt16(getfData2tableList(1,0),&dbuf[32]);   //中区1#机泵电流
    fv2UIntt16(getfData2tableList(1,1),&dbuf[34]);   //中区2#机泵电流
    fv2UIntt16(getfData2tableList(1,2),&dbuf[36]);   //中区3#机泵电流
    fv2UIntt16(getfData2tableList(1,3),&dbuf[38]);   //中区变频器频率
    fv2UIntt16(getfData2tableList(1,20),&dbuf[40]);   //中区设定压力
    fv2UIntt16(getfData2tableList(1,5),&dbuf[42]);   //中区出水压力
    dv2UIntt32(getdData2tableList(1,9),&dbuf[44]);   //中区电量行度  44 45 46 47
    dbuf[48] = 0x00;dbuf[49] = 0x00;dbuf[50] = 0x00;dbuf[51] = 0x00;  //备用

    //高区机泵状态
    //fv2UIntt16(getfData2tableList(2,10),&dbuf[52]);   //高区机泵状态
    fv2UIntt16(getfData2tableList(2,0),&dbuf[54]);   //高区1#机泵电流
    fv2UIntt16(getfData2tableList(2,1),&dbuf[56]);   //高区2#机泵电流
    fv2UIntt16(getfData2tableList(2,2),&dbuf[58]);   //高区3#机泵电流
    fv2UIntt16(getfData2tableList(2,3),&dbuf[60]);   //高区变频器频率
    fv2UIntt16(getfData2tableList(2,20),&dbuf[62]);   //高区设定压力
    fv2UIntt16(getfData2tableList(2,5),&dbuf[64]);   //高区出水压力
    dv2UIntt32(getdData2tableList(2,9),&dbuf[66]);   //高区电量行度  66 67 68 69
    dbuf[70] = 0x00;dbuf[71] = 0x00;dbuf[72] = 0x00;dbuf[73] = 0x00;  //备用

    //超高区机泵状态
    //fv2UIntt16(getfData2tableList(3,10),&dbuf[74]);   //超高区机泵状态
    fv2UIntt16(getfData2tableList(3,0),&dbuf[76]);   //超高区1#机泵电流
    fv2UIntt16(getfData2tableList(3,1),&dbuf[78]);   //超高区2#机泵电流
    fv2UIntt16(getfData2tableList(3,2),&dbuf[80]);   //超高区3#机泵电流
    fv2UIntt16(getfData2tableList(3,3),&dbuf[82]);   //超高区变频器频率
    fv2UIntt16(getfData2tableList(3,20),&dbuf[84]);   //超高区设定压力
    fv2UIntt16(getfData2tableList(3,5),&dbuf[86]);   //超高区出水压力
    dv2UIntt32(getdData2tableList(3,9),&dbuf[88]);   //超高区电量行度  88 89 90 91
    dbuf[92] = 0x00;dbuf[93] = 0x00;dbuf[94] = 0x00;dbuf[95] = 0x00;  //备用

    calculationValue(getdData2tableList(0,9),getdData2tableList(1,9),getdData2tableList(2,9),getdData2tableList(3,9),&dbuf[96]);//96 97 98 99 总电量行度
    fv2UIntt16(getfData2tableList(0,6),&dbuf[100]);   //低区区进水压力

}

void    DataCenter::waterStatus(int pos,ST_BYTE *pbuf)
{
    if(pos== 1)
    {
        if(getfData2tableList(0,15)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(0,15)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(0,15)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(0,16)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(0,16)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(0,16)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(0,17)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(0,17)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(0,17)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(0,19)==1)pbuf[0] = pbuf[0] |  0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(0,10)==2)pbuf[0] = pbuf[0] |  0x08;   //低区变频器故障
    }
    if(pos== 2)
    {//中区
        if(getfData2tableList(1,15)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(1,15)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(1,15)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(1,16)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(1,16)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(1,16)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(1,17)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(1,17)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(1,17)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(1,19)==1)pbuf[0] = pbuf[0] |  0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(1,10)==2)pbuf[0] = pbuf[0] |  0x08;   //低区变频器故障
    }
    if(pos== 3)
    {//高区
        if(getfData2tableList(2,15)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(2,15)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(2,15)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(2,16)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(2,16)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(2,16)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(2,17)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(2,17)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(2,17)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(2,19)==1)pbuf[0] = pbuf[0] |  0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(2,10)==2)pbuf[0] = pbuf[0] |  0x08;   //低区变频器故障
    }
    if(pos== 4)
    {
        if(getfData2tableList(3,15)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(3,15)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(3,15)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(3,16)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(3,16)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(3,16)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(3,17)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(3,17)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(3,17)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(3,19)==1)pbuf[0] = pbuf[0] |  0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(3,10)==2)pbuf[0] = pbuf[0] |  0x08;   //低区变频器故障
    }
}
