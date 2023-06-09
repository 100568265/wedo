#include "Manager.h"
#include "tinyxml2.h"
#include <iostream>
#include "TcpClientPort.h"
#include "remoteProtocol.h"
 #include <unistd.h>

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
    delete m_client;
    delete m_protocol;
}


int    Manager::load_configs()
{
    //cout << "Hello world!" << endl;
    XMLDocument doc;
	if(doc.LoadFile("comm_configparam.xml")!=0)
	{
		cout<<"load xml file failed"<<endl;
		return -1;
	}
	else
    {
        cout<<"load xml file successful"<<endl;
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

    return true;
}

int    Manager::start()
{
    int is_local = memcmp(m_ipaddr,"127.0.0.1",sizeof("127.0.0.1"));
    if(!is_local)
    {
        printf("localhost,stop service!");
        return 0;
    }
    m_client = new TcpClientPort(m_ipaddr,m_port,m_devName);
    m_client->Open();

    m_protocol = new remoteProtocol(m_client);

    while(1)
    {
        unsigned char buf[1024];
        int  len=0;
        if(m_client->m_isopen){
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
    return 1;
}

void   Manager::checkLastTime()
{
    time(&new_Time);
    if(difftime(new_Time,rec_Time)>60*2) {
        printf("No message for more than two minutes!Close client.\r\n");
        m_client->Close();
    }
}













































