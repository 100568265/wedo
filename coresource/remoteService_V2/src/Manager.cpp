#include "Manager.h"
#include "tinyxml2.h"
#include <iostream>
#include "TcpClientPort.h"
#include "remoteProtocol.h"
 #include <unistd.h>
#include "syslogger.h"

using namespace tinyxml2;
using namespace std;
Manager::Manager():
m_client(NULL),
m_protocol(NULL)
{
    //ctor
}

Manager::~Manager()
{
    //dtor
    m_client->Close();
    m_MonitorClient->Close();//报文监听客户端
    m_tranferClient->Close();
    delete m_client;
    delete m_protocol;
}


int    Manager::load_configs()
{
    //cout << "Hello world!" << endl;
    XMLDocument doc;
	if(doc.LoadFile("/root/comm/config/comm_configparam.xml")!=0)
	{
		cout<<"load xml file failed"<<endl;
		SysLogger::GetInstance()->LogError("load xml file failed\n");
		return -1;
	}
	else
    {
        cout<<"load xml file successful"<<endl;
        SysLogger::GetInstance()->LogInfo("load xml file successful\n");
    }
    XMLElement* elmtRoot = doc.RootElement();
    XMLElement *elmtIP = elmtRoot->FirstChildElement("IP");
    XMLElement *elmtPort = elmtRoot->FirstChildElement("Port");
    XMLElement *elmtName = elmtRoot->FirstChildElement("Name");
    //elmtIP->GetText();                      //获取IP地址
    m_port   = atoi(elmtPort->GetText()); //获取端口号

    memset(m_ipaddr,0,sizeof(m_ipaddr));
    memcpy(m_ipaddr,elmtIP->GetText(),strlen(elmtIP->GetText()));

    memset(m_devName,0,sizeof(m_devName));
    memcpy(m_devName,elmtName->GetText(),strlen(elmtName->GetText()));

    //printf("xml data {ipaddr : %s ,port:%s devName : %s}\n",elmtIP->GetText(),elmtPort->GetText(),elmtName->GetText());
    //printf("manager  data {ipaddr : %s ,port:%d devName : %s}\n",m_ipaddr,m_port,m_devName);
    return true;
}

int    Manager::start()
{
    int is_local = memcmp(m_ipaddr,"127.0.0.1",sizeof("127.0.0.1"));
    if(!is_local)
    {
        printf("localhost,stop service!\n");
        return 0;
    }
    m_client = new TcpClientPort(m_ipaddr,m_port,m_devName);
    m_client->Open();

    m_protocol = new remoteProtocol(this,m_client);
    do{
        onProcess();
        sleep(0.5);
    }while(1);
    return 1;
}

void    Manager::onProcess()
{
    //printf("inter onProcess\n");
    unsigned char buf[1024];
    int  len=0;
    if(m_client->isOpen()){
        sleep(1);

        int res = m_client->recv_Buf(buf,len);
        if(res){
            ST_BYTE *recvbuf = new ST_BYTE[len];
            memcpy(recvbuf,buf,len);
            m_protocol->process_receive(recvbuf,len);
            delete recvbuf;
            time(&rec_Time);
        }
        checkLastTime();
    }
    else{
        sleep(10);
        m_client->Open();
    }
}

void   Manager::checkLastTime()
{
    time(&new_Time);
    if(difftime(new_Time,rec_Time)>60*3) {
        printf("No message for more than eight minutes!Close client.\r\n");
        SysLogger::GetInstance()->LogWarn("No message for more than eight minutes!Close client.\n");
        m_client->Close();
        time(&rec_Time);
    }
}

bool   Manager::startMonitor(int port)
{
    m_MonitorClient = new TcpClientPort("127.0.0.1",9999,"Monitor");
    m_MonitorClient->Open();
    m_tranferClient = new TcpClientPort(m_client->get_Ipaddr(),port,"Tranfer");
    m_tranferClient->Open();
    if(m_MonitorClient->isOpen()&&m_tranferClient->isOpen()){
        m_MonitorThread.Start(MonitorTaskProc,this,true);
        m_ReceiveThread.Start(ReceiveTaskProc,this,true);
        return true;
    }
    return false;
}

ST_VOID* Manager::MonitorTaskProc(ST_VOID *param)
{
    Manager *pd = (Manager*)param;
    pd->transfer_Monitor();
    sleep(0.1);
    return 0;
}

ST_VOID* Manager::ReceiveTaskProc(ST_VOID *param)
{
    Manager *pd = (Manager*)param;
    pd->Recive_transfer();
    sleep(0.1);
    return 0;
}



void    Manager::transfer_Monitor()
{
    try{
        if(m_MonitorClient->isOpen() && m_tranferClient->isOpen())
        {
            unsigned char buf[1024];
            int  len=0;
            int res = m_MonitorClient->recv_Buf(buf,len);
            if(res){
                time(&rec_Time);   //如果在此处不更新时间，将会导致主线程超过时间重连服务器
                int sres = m_tranferClient->send_Buf(buf,len);
                if(!sres)printf("send error!\n");
            }
        }
        else
        {
            sleep(2);
            printf("tcp connect is close\n");

        }
    }catch(...){
        printf("catch error\n");
    }

}

void    Manager::Recive_transfer()
{
    unsigned char buf[1024];
    int  len=0;
    if(m_tranferClient->isOpen()){
        sleep(1);

        int res = m_tranferClient->recv_Buf(buf,len);
        if(res){
        ST_BYTE *recvbuf = new ST_BYTE[len];
        memcpy(recvbuf,buf,len);
        printf("Recive_transfer : ");
        for(int i =0;i<len;i++){
           printf("0x02%X",recvbuf[i]);
        }
        printf("\n");
        delete recvbuf;
        time(&rec_Time);
        }
    }
    else{
        m_MonitorClient->Close();
        m_tranferClient->Close();
        m_MonitorThread.Stop();
        m_ReceiveThread.Stop();
        printf("Stop Monitor thread\n");
    }

}
