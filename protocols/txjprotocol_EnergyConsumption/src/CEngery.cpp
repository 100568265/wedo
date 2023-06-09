#include "CEngery.h"
#include <vector>
#include <string>
#include <string.h>
#include "AES.h"
#include "syslogger.h"
#include "BUFDataCache.h"

extern NodeTree *g_pTree;
static string keys = "aes.modeaes.mode";

using namespace std;
CEngery::CEngery()
{
    //ctor
    bdCache = new BUFDataCache();
    m_regesit = false;
    time(&heat_Newtime);
	time(&heat_Oldtime);
	time(&sdata_Newtime);
	time(&sdata_Oldtime);
	m_WorkTread.Start(WorkTaskProc,this,true);
}

CEngery::~CEngery()
{
    //dtor
}

vector<string> split(const string &str, const string &pattern)
{
    vector<string> res;
    if(str == "")
        return res;
    //在字符串末尾也加入分隔符，方便截取最后一段
    string strs = str + pattern;
    size_t pos = strs.find(pattern);

    while(pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        res.push_back(temp);
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos+1, strs.size());
        pos = strs.find(pattern);
    }

    return res;
}

CEngery* CreateInstace()
{
    return new CEngery();
}

void	CEngery::Init()
{
    const DeviceInfo & minfo = *this->GetDevice()->GetDeviceInfo();
    string info_secret(minfo.Addressex);
    vector<string> posvec;
    posvec = split(info_secret,"|");
    if(posvec.size()<2)
        return;
    Strcpy(building_id,posvec[0].c_str());
    Strcpy(gateway_id,posvec[1].c_str());
}
void	CEngery::Uninit()
{

}
void	CEngery::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
    int32_t	len = this->GetCurPort()->PickBytes(pbuf, 4, 2000);
    if(len < 4) {
        this->GetCurPort()->Clear();
        return;
    }
    int enclen = 0;
/*    enclen = (enclen & (pbuf[0]<<24));
    enclen = (enclen & (pbuf[1]<<16));
    enclen = (enclen & (pbuf[2]<<8));
    enclen = (enclen & (pbuf[3]));*/
    enclen = pbuf[0]*256*256*256+pbuf[1]*256*256+pbuf[2]*256+pbuf[3];
//    Memcpy(&enclen,pbuf,4);
    enclen = enclen+4;
    char tmp[32];
    sprintf(tmp, "enclen:%d", enclen);
    ShowMessage(tmp);
    len = this->GetCurPort()->ReadBytes(pbuf,enclen,1000);
    if(enclen == len)
    {
        readed = len;
        return ;
    }
    else
    {
        this->ShowMessage("datelen is error");
        this->GetCurPort()->Clear();
        return ;
    }
    return;

}

ST_BOOLEAN	CEngery::OnSend()
{
    if(!m_regesit)
    {
        send_id_validate();
        //m_regesit = true;
        return 1;
    }

    time(&heat_Newtime);
    if(difftime(heat_Newtime, heat_Oldtime)>60*1)//一分钟发送一次心跳
    {
        this->ShowMessage("send heatbeat");
        time(&heat_Oldtime);
        send_heart_beat();
        //send_id_validate();
        return 1;
    }

    time(&sdata_Newtime);
    if(difftime(sdata_Newtime, sdata_Oldtime)>60*3)//五分钟发送一次数据
    {
        this->ShowMessage("send data!");
        time(&sdata_Oldtime);
        send_data();
        return 1;
    }

    return 1;
}

void	CEngery::sendTest()
{
    string xmlstr = "<root><common><building_id>XXXXXXXXXX</building_id ><gateway_id>XX</gateway_id><type>result</type></common><id_validate operation=\"result\"><result>pass</result></id_validate></root>";
    aes_text in_text;
	aes_text out_text;

	in_text.content = new char[xmlstr.length()];
	memset(in_text.content, 0, xmlstr.length());
	strcpy(in_text.content, xmlstr.c_str());
	in_text.len = xmlstr.length();

    aes_encrypt_Hex((char *)keys.c_str(), &in_text, &out_text);

    ST_INT32 byteLen = out_text.len/2;
    ST_BYTE *encbuf = new ST_BYTE[byteLen];
    ST_BYTE *sendbuf =new ST_BYTE[byteLen+4];
	//cout << "解密后的十六进制数据： " << out_text.content << endl;
    hexToUChar(out_text.content,encbuf);
    sendbuf[0] = byteLen>>24;
    sendbuf[1] = byteLen>>16;
    sendbuf[2] = byteLen>>8;
    sendbuf[3] = byteLen;
    Memcpy(&sendbuf[4],encbuf,byteLen);
    this->Send(sendbuf,byteLen+4);

    delete in_text.content;
    delete out_text.content;
    delete[] sendbuf;
    delete[] encbuf;
}

ST_BOOLEAN	CEngery::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
    int enclen = 0;
    enclen = pbuf[0]*256*256*256+pbuf[1]*256*256+pbuf[2]*256+pbuf[3];

 //   char *hexChar = new char[(enclen*2)+1];

    aes_text in_text,out_text;
    in_text.content = new char[(enclen*2)+1];
    //ucharToHex(&pbuf[4],in_text.content);
    ucharToHexLen(&pbuf[4],enclen,in_text.content);


    in_text.len = (enclen*2);
    aes_decrypt_Hex((char *)keys.c_str(),&in_text,&out_text);
//    ShowMessage(in_text.content);
//    ShowMessage(out_text.content);
    anaylizeXmlData(out_text.content);

//    delete []hexChar;
    delete in_text.content;
    delete out_text.content;

    return 1;
}

ST_BOOLEAN	CEngery::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

string getTime() {
    time_t tt = time(NULL);
    struct tm *stm = localtime(&tt);

    char tmp[32];
    sprintf(tmp, "%04d%02d%02d%02d%02d%02d", 1900 + stm->tm_year, 1 + stm->tm_mon, stm->tm_mday, stm->tm_hour,
            stm->tm_min, stm->tm_sec);

    return tmp;
}

void       CEngery::anaylizeXmlData(char *xmlData)
{
    TiXmlDocument* myDocument = new TiXmlDocument();
    myDocument->Parse(xmlData);
    TiXmlElement *rootelemt = myDocument->RootElement();

    if(rootelemt!=NULL)
    {
        TiXmlElement *commElemt = rootelemt->FirstChildElement("common");
        if(commElemt!=NULL)
        {
            TiXmlElement *typeElemt = commElemt->FirstChildElement("type");
            if(typeElemt!=NULL)
            {
                string typeStr = typeElemt->GetText();
                if(typeStr == "period")
                {
                    ShowMessage("receive period");
                    resend_config();
                }
                else if(typeStr == "result")
                {
                    TiXmlElement *idElemt = rootelemt->FirstChildElement("id_validate");
                    if(idElemt != NULL)
                    {
                        TiXmlElement *resultElemt = idElemt->FirstChildElement("result");
                        if(resultElemt != NULL)
                        {
                            string resstr = resultElemt->GetText();
                            if(resstr == "pass")
                            {
                                ShowMessage("receive regesit pass");
                                m_regesit = true;
                            }
                        }
                    }
                }
            }
        }
    }
    delete myDocument;
}

TiXmlElement *  CEngery::creat_RootElemt(const char *typeText)
{
    TiXmlElement *pRootElement = new TiXmlElement("root");

    TiXmlElement* commElmt = new TiXmlElement("common");
    TiXmlElement* bIdElmt = new TiXmlElement("building_id");
    TiXmlElement* gIdElmt = new TiXmlElement("gateway_id");
    TiXmlElement* typeElmt = new TiXmlElement("type");
/*    bIdElmt->SetValue("10016");
    gIdElmt->SetValue("45698");
    typeElmt->SetValue("request");*/
    bIdElmt->LinkEndChild(new TiXmlText(building_id));
    gIdElmt->LinkEndChild(new TiXmlText(gateway_id));
    typeElmt->LinkEndChild(new TiXmlText(typeText));

    commElmt->LinkEndChild(bIdElmt);
    commElmt->LinkEndChild(gIdElmt);
    commElmt->LinkEndChild(typeElmt);
    pRootElement->LinkEndChild(commElmt);

    return pRootElement;
}

TiXmlElement *  CEngery::creat_elemt(string name,string txt)
{
    TiXmlElement *tmpElemt = new TiXmlElement(name.c_str());
    if(txt == "")
        return tmpElemt;

    tmpElemt->LinkEndChild(new TiXmlText(txt.c_str()));
    return tmpElemt;

}

void    CEngery::send_id_validate()
{
    TiXmlDocument* pWriteDoc = new TiXmlDocument();
    TiXmlElement * pRootElement = creat_RootElemt("request");

    TiXmlElement* idvaElmt = new TiXmlElement("id_validate");
    idvaElmt->SetAttribute("operation","request");
    TiXmlElement* resElmt = new TiXmlElement("result");
    idvaElmt->LinkEndChild(resElmt);
    pRootElement->LinkEndChild(idvaElmt);

    pWriteDoc->LinkEndChild(pRootElement);

    //string keys = "aes.modeaes.mode";
    TiXmlPrinter printer;
    string xmlstr;

    pWriteDoc->Accept(&printer);
    xmlstr = printer.CStr();
//    cout<<xmlstr<<endl;

    aes_text in_text;
	aes_text out_text;

	in_text.content = new char[xmlstr.length()];
	memset(in_text.content, 0, xmlstr.length());
	strcpy(in_text.content, xmlstr.c_str());
	in_text.len = xmlstr.length();

    aes_encrypt_Hex((char *)keys.c_str(), &in_text, &out_text);
    //SysLogger::GetInstance()->LogDebug(out_text.content);

    ST_INT32 byteLen = out_text.len/2;
    ST_BYTE *encbuf = new ST_BYTE[byteLen];
    ST_BYTE *sendbuf = new ST_BYTE[byteLen+4];
	//cout << "解密后的十六进制数据： " << out_text.content << endl;
    hexToUChar(out_text.content,encbuf);
    sendbuf[0] = byteLen>>24;
    sendbuf[1] = byteLen>>16;
    sendbuf[2] = byteLen>>8;
    sendbuf[3] = byteLen;
    Memcpy(&sendbuf[4],encbuf,byteLen);
    this->Send(sendbuf,byteLen+4);

/*    delete idvaElmt;
    delete resElmt;
    delete pRootElement;*/

    delete pWriteDoc;
    delete in_text.content;
    delete out_text.content;
    delete[] sendbuf;
    delete[] encbuf;
}

void CEngery::send_heart_beat()
{
    TiXmlDocument* pWriteDoc = new TiXmlDocument();
    TiXmlElement * pRootElement = creat_RootElemt("notify");

    TiXmlElement* idvaElmt = new TiXmlElement("heart_beat");
    idvaElmt->SetAttribute("operation","notify");
    TiXmlElement* timeElmt = new TiXmlElement("time");
    idvaElmt->LinkEndChild(timeElmt);
    pRootElement->LinkEndChild(idvaElmt);

    pWriteDoc->LinkEndChild(pRootElement);

    //string keys = "aes.modeaes.mode";
    TiXmlPrinter printer;
    string xmlstr;

    pWriteDoc->Accept(&printer);
    xmlstr = printer.CStr();
//    cout<<xmlstr<<endl;

    aes_text in_text;
	aes_text out_text;

	in_text.content = new char[xmlstr.length()];
	memset(in_text.content, 0, xmlstr.length());
	strcpy(in_text.content, xmlstr.c_str());
	in_text.len = xmlstr.length();

    aes_encrypt_Hex((char *)keys.c_str(), &in_text, &out_text);

    ST_INT32 byteLen = out_text.len/2;
    ST_BYTE *encbuf = new ST_BYTE[byteLen];
    ST_BYTE *sendbuf =new ST_BYTE[byteLen+4];
	//cout << "解密后的十六进制数据： " << out_text.content << endl;
    hexToUChar(out_text.content,encbuf);
    sendbuf[0] = byteLen>>24;
    sendbuf[1] = byteLen>>16;
    sendbuf[2] = byteLen>>8;
    sendbuf[3] = byteLen;
    Memcpy(&sendbuf[4],encbuf,byteLen);
    this->Send(sendbuf,byteLen+4);

    delete pWriteDoc;

    delete in_text.content;
    delete out_text.content;
    delete[] sendbuf;
    delete[] encbuf;
}


void CEngery::resend_config()
{
    TiXmlDocument* pWriteDoc = new TiXmlDocument();
    TiXmlElement * pRootElement = creat_RootElemt("period_ack");

    TiXmlElement* idvaElmt = new TiXmlElement("config");
    idvaElmt->SetAttribute("operation","period_ack");
    TiXmlElement* timeElmt = new TiXmlElement("period");
    idvaElmt->LinkEndChild(timeElmt);
    pRootElement->LinkEndChild(idvaElmt);

    pWriteDoc->LinkEndChild(pRootElement);

    //string keys = "aes.modeaes.mode";
    TiXmlPrinter printer;
    string xmlstr;

    pWriteDoc->Accept(&printer);
    xmlstr = printer.CStr();
//    cout<<xmlstr<<endl;

    aes_text in_text;
	aes_text out_text;

	in_text.content = new char[xmlstr.length()];
	memset(in_text.content, 0, xmlstr.length());
	strcpy(in_text.content, xmlstr.c_str());
	in_text.len = xmlstr.length();

    aes_encrypt_Hex((char *)keys.c_str(), &in_text, &out_text);

    ST_INT32 byteLen = out_text.len/2;
    ST_BYTE *encbuf = new ST_BYTE[byteLen];
    ST_BYTE *sendbuf =new ST_BYTE[byteLen+4];
	//cout << "解密后的十六进制数据： " << out_text.content << endl;
    hexToUChar(out_text.content,encbuf);
    sendbuf[0] = byteLen>>24;
    sendbuf[1] = byteLen>>16;
    sendbuf[2] = byteLen>>8;
    sendbuf[3] = byteLen;
    Memcpy(&sendbuf[4],encbuf,byteLen);
    this->Send(sendbuf,byteLen+4);

    delete pWriteDoc;

    delete in_text.content;
    delete out_text.content;
    delete[] sendbuf;
    delete[] encbuf;
}

void CEngery::send_data()
{
     ////////////////////////////////////////////////////////////////////////
	ST_INT j = 0;
	ST_CHAR devname[64] = {0}, ditname[64] = {0}, fullname[256] = {0};
    ST_INT  devcount = g_pTree->GetNameNodeCount(NULL);  //获取设备数量
	for(ST_INT i = 0; i < devcount; ++i)
	{
		if (g_pTree->GetNodeName("", i, devname) < 0)
            continue;

		ST_INT varcount = g_pTree->GetNameNodeCount(devname);//获取该设备的变量数目
        if (varcount <= 0) continue;

        if(strcmp(devname,"channelstate")==0|strcmp(devname,"devicestate")==0|strcmp(devname,"engery")==0) //将设备状态和通道状态去除
                continue;

        TiXmlDocument* pWriteDoc = new TiXmlDocument();
        TiXmlElement * pRootElement = creat_RootElemt("report");

        TiXmlElement* dataElmt = new TiXmlElement("data");
        dataElmt->SetAttribute("operation","report");
        char stmp[32];
        sprintf(stmp, "%08d", sequence++);
        dataElmt->LinkEndChild(creat_elemt("sequence",stmp));
        dataElmt->LinkEndChild(creat_elemt("parser","yes"));
        dataElmt->LinkEndChild(creat_elemt("time",getTime()));


        TiXmlElement *meterElmt = new TiXmlElement("meter");
        int meterID = 0;
        //meterElmt->SetAttribute("id",to_string(tdd.device));
		for(j = 0; j < varcount; ++j)
		{
//            memset(fullname, 0, sizeof(fullname) / 2);
//            memset(ditname , 0, sizeof(ditname ) / 2);
            *(int32_t*)fullname = 0; *(int32_t*)ditname = 0;

		    g_pTree->GetNodeName(devname, j, ditname);
            string ditstr = ditname;
            int dataID = get_data_id(ditstr);
            if(dataID == -1)
                continue;

            strcpy(fullname, devname); strcat(fullname, ".");
		    strcat(fullname, ditname); strcat(fullname, ".value");

		    ST_DUADDR tdd;
		    g_pTree->GetNodeAddr(fullname, tdd);
		    meterID = tdd.device;

            ST_VARIANT vValue;
            // GetVariableValueByName(&fullname[0],vValue);
            GetVariableValueByAddr(tdd, vValue);

            TiXmlElement *functionElmt = new TiXmlElement("function");
            functionElmt->SetAttribute("id",to_string(dataID).c_str());
            functionElmt->SetAttribute("coding","01000");
            functionElmt->SetAttribute("error","0");
            switch(vValue.vt)
            {
            case VALType_Float:
                {
                    ST_FLOAT Value  = vValue.fVal;
                    functionElmt->LinkEndChild(new TiXmlText(to_string(Value).c_str()));
                }
                break;
            case VALType_Double:
                {
                    ST_DOUBLE Value  = vValue.dtVal;
                    functionElmt->LinkEndChild(new TiXmlText(to_string(Value).c_str()));
                }
                break;
            default:
                break;
            }
            meterElmt->LinkEndChild(functionElmt);

        }
        meterElmt->SetAttribute("id",to_string(meterID).c_str());
        dataElmt->LinkEndChild(meterElmt);
        pRootElement->LinkEndChild(dataElmt);
        pWriteDoc->LinkEndChild(pRootElement);

        TiXmlPrinter printer;
        string xmlstr;

        pWriteDoc->Accept(&printer);
        xmlstr = printer.CStr();
 //       ShowMessage("test");
 //       ShowMessage(xmlstr.c_str());
        send_encrypt_Data(xmlstr);
        sleep(1);
        delete pWriteDoc;
	}


}

void CEngery::send_encrypt_Data(string xmlstr)
{
    aes_text in_text;
	aes_text out_text;

	in_text.content = new char[xmlstr.length()];
	memset(in_text.content, 0, xmlstr.length());
	strcpy(in_text.content, xmlstr.c_str());
	in_text.len = xmlstr.length();

    aes_encrypt_Hex((char *)keys.c_str(), &in_text, &out_text);

    ST_INT32 byteLen = out_text.len/2;
    ST_BYTE *encbuf = new ST_BYTE[byteLen];
    ST_BYTE *sendbuf =new ST_BYTE[byteLen+4];
	//cout << "解密后的十六进制数据： " << out_text.content << endl;
    hexToUChar(out_text.content,encbuf);
    sendbuf[0] = byteLen>>24;
    sendbuf[1] = byteLen>>16;
    sendbuf[2] = byteLen>>8;
    sendbuf[3] = byteLen;
    Memcpy(&sendbuf[4],encbuf,byteLen);
    this->send_buf(sendbuf,byteLen+4);

    delete in_text.content;
    delete out_text.content;
    delete[] sendbuf;
    delete[] encbuf;
}

int CEngery::get_data_id(string type)
{
    if(type == "总有功电度")
        return 1;
    else if(type == "A相电流")
        return 2;
    else if(type == "B相电流")
        return 3;
    else if(type == "C相电流")
        return 4;
    else if(type == "A相电压")
        return 5;
    else if(type == "B相电压")
        return 6;
    else if(type == "C相电压")
        return 7;
    else if(type == "总有功功率")
        return 8;
    else if(type == "总功率因素")
        return 9;
    else
        return -1;
    return -1;
}

void CEngery::send_buf(ST_BYTE *pBuf,ST_INT len)
{
    int res = this->Send(pBuf,len);
    if(!res)
    {
        ShowMessage("send message error!");
        bdCache->pushBUF(pBuf,len);
        isOpen = false;
    }
    else
    {
        ShowMessage("send message successful!");
        isOpen = true;
    }
}

void  CEngery::check_data_send()
{
    if(isOpen)
    {
        if(bdCache->getSize()>0)
        {
            while(bdCache->getSize()>0){
                ST_BYTE pbuf[2048] ;
                Memset(pbuf,0,2048);
                int pSize = 0;
                bdCache->get_front_BUF(pbuf,pSize);
                bool sRes = false;
                if(pSize>0)
                    sRes = this->Send(pbuf,pSize);

                if(sRes)
                    bdCache->pop();  //发送成功则弹出队列第一个元素
                else{
                    isOpen = false;
                    return;   //发送失败则退出函数
                }

            }
        }
    }
    sleep(1);
}

ST_VOID *CEngery::WorkTaskProc(ST_VOID *param)
{
    CEngery *pro = (CEngery *)param;
    pro->check_data_send();
	return 0;
}






















