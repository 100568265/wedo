#include "remoteProtocol.h"
#include "TcpClientPort.h"
#include <string.h>
#include <string>
#include <sstream>
#include <iconv.h>
 #include <fcntl.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sysTools.h>
#include "syslogger.h"
#include "Manager.h"


remoteProtocol::remoteProtocol(Manager *manager,TcpClientPort *client):
    m_client(client),
    m_manager(manager)
{
    //ctor
    InitFilePathMap();
}

remoteProtocol::~remoteProtocol()
{
    //dtor
}


void      remoteProtocol::InitFilePathMap()
{
    //etc/comm/variabletransformer.xml
    m_filepathMap.insert(std::pair<string,string>("comm_network.xml","/root/comm/config/comm_network.xml"));
    m_filepathMap.insert(std::pair<string,string>("comm_configparam.xml","/root/comm/config/comm_configparam.xml"));
    m_filepathMap.insert(std::pair<string,string>("variabletransformer.xml","/etc/comm/variabletransformer.xml"));
    m_filepathMap.insert(std::pair<string,string>("transformerprotocoltype.xml","/etc/comm/transformerprotocoltype.xml"));
    m_filepathMap.insert(std::pair<string,string>("template.xml","/etc/comm/template.xml"));
    m_filepathMap.insert(std::pair<string,string>("channel.xml","/etc/comm/channel.xml"));
    m_filepathMap.insert(std::pair<string,string>("log.log","/root/comm/log.log"));
}

/*
void      remoteProtocol::InitFilePathMap()
{
    //etc/comm/variabletransformer.xml
    m_filepathMap.insert(std::pair<string,string>("comm_network.xml","test/comm_network.xml"));
    m_filepathMap.insert(std::pair<string,string>("comm_configparam.xml","test/comm_configparam.xml"));
    m_filepathMap.insert(std::pair<string,string>("variabletransformer.xml","test/variabletransformer.xml"));
    m_filepathMap.insert(std::pair<string,string>("transformerprotocoltype.xml","test/transformerprotocoltype.xml"));
    m_filepathMap.insert(std::pair<string,string>("template.xml","test/template.xml"));
    m_filepathMap.insert(std::pair<string,string>("channel.xml","test/channel.xml"));
    m_filepathMap.insert(std::pair<string,string>("log.log","test/log.log"));
}*/

string remoteProtocol::getfilePath(string filename)
{
    string path = "NULL";

    map<string,string>::iterator it;
    it = m_filepathMap.find(filename);
    if (it != m_filepathMap.end())
        path = it->second;

    return path;

}



int    remoteProtocol::onRead(ST_BYTE *pbuf,int& readed)
{
    if(sizeof(pbuf)<4)
        return -1;
    if(pbuf[0]==0x0A&&pbuf[4]==0x0B)
    {
        int pufLen = pbuf[2]*256+pbuf[3]+7;

        ST_BYTE sumByte = 0x00;

        for(int i=0;i<(pufLen-2);i++)
        {
            sumByte += pbuf[i];
        }

        if(sumByte == pbuf[pufLen-2])
        {
            readed = pufLen;
            return 1;
        }

        printf("check error\n");
        return -1;
    }
    else
    {
        printf("Message format error\n");
        return -1;
    }
}

int   remoteProtocol::Send(ST_BYTE *pbuf,int len)
{
 //   int pbufSize = sizeof(pbuf);
 /*   if(pbufSize<len)
        return -1;*/
    ST_BYTE *sendbuf = new  ST_BYTE[len+1];
    memcpy(sendbuf,pbuf,len);
    bool res = m_client->send_Buf(sendbuf,len);
    printf("Send:    ");
    for(int i=0;i<len;i++)
    {
        printf("%02X ",sendbuf[i]);
    }
    printf("\r\n");
    delete[] sendbuf;
    return res;
}

void   remoteProtocol::process_receive(ST_BYTE *bpuf,int dataLen)
{
    int res =0 , plen = 0;
    res = onRead(bpuf,plen);
    if(res != -1)
    {
        printf("receive: ");
        for(int i=0 ; i<plen;i++)
        {
            printf("%02X ",bpuf[i]);
        }
        printf("\r\n");
        replay_Server(bpuf);
    }
    else{
        request_resend();
    }

}


int    remoteProtocol::replay_Server(ST_BYTE* bpuf)
{
    ST_BYTE code = 0x00;
    code = bpuf[1];
    switch(code){
    case 0x01:
        {
            parsing_01(bpuf);
        }break;
    case 0x02:
        {
            parsing_02(bpuf);
        }break;
    case 0x03:
        {
            parsing_03(bpuf);
        }break;
    }
    return true;

}

void    remoteProtocol::parsing_01(ST_BYTE *pbuf)
{
    int datalen  = pbuf[2]*256+pbuf[3];
    int fileNameSize = datalen - 1;
    char *fileName = new char[fileNameSize+1];
    memcpy(fileName,&pbuf[6],fileNameSize);      //hex 2 char
    fileName[fileNameSize] = '\0';
    printf("file name :%s\n",fileName);
    m_filename = fileName;
    send_fileSize(fileName);
    delete fileName;
}

void    remoteProtocol::parsing_02(ST_BYTE *pbuf)
{
    int dataSize = pbuf[2]*256 + pbuf[3];
    int fileSize = pbuf[6]*256*256*256 + pbuf[7]*256*256 + pbuf[8]*256 +pbuf[9];
    if(fileSize<=0)
        return;

    int fileNameSize = dataSize- 5;
    char *fName = new char[fileNameSize+1];
    memcpy(fName,&pbuf[10],fileNameSize);      //hex 2 char
    fName[fileNameSize] = '\0';

    ST_CHAR fullname[256] = {0};     //文件名拼接
    strcpy(fullname,"temp/");
    strcat(fullname,fName);

    printf("filePath :%s\r\n",fullname);
    SysLogger::GetInstance()->LogInfo("filePath :%s\r\n",fullname);

    ST_BYTE buf1[] = {0x0A,0x03,0x00,0x02,0x0B,0x01,0x01,0x1C,0x16};//0A 03 01 01 0B 01 CS 16
    this->Send(buf1,sizeof(buf1));  //回复准备接收

    int res = m_client->rece_File((const char*)fullname,fileSize);  //接收文件
    if(res)
    {
        ST_BYTE buf2[] = {0x0A,0x03,0x00,0x02,0x0B,0x01,0x02,0x1D,0x16};//0A 03 01 01 0B 02 CS 16
        this->Send(buf2,sizeof(buf2));  //回复接收成功
    }
    else
    {
        ST_BYTE buf3[] = {0x0A,0x03,0x00,0x02,0x0B,0x01,0x03,0x1E,0x16};//0A 03 01 01 0B 02 CS 16
        this->Send(buf3,sizeof(buf3));  //回复接收失败
    }
    delete[] fName;

}

//0A 01 05 00 0b
void    remoteProtocol::parsing_03(ST_BYTE *pbuf)
{
    ST_BYTE funType = pbuf[5];
    switch(funType){
    case 0x01:
        {
            ST_BYTE bvalue = pbuf[6];
            if(bvalue == 0x01){
                m_client->send_File(getfilePath(m_filename).c_str());
                return ;
            }
            else if(bvalue == 0x04){
                sysTools::DeleteFile("temp");
                ST_BYTE buf[] = {0x0A,0x03,0x00,0x02,0x0B,0x01,0x05,0x20,0x16};//0A 03 01 01 0B 02 CS 16
                this->Send(buf,sizeof(buf));  //回复接收成功
                return ;
            }

        }break;
    case 0x02: //重启管理机
        {
            ST_BYTE buf2[] = {0x0A,0x03,0x00,0x02,0x0B,0x02,0x02,0x1E,0x16};//0A 03 01 01 0B 02 CS 16
            this->Send(buf2,sizeof(buf2));  //回复接收成功
            sleep(6);
            m_client->Close();
            sleep(2);
            /* 同步磁盘数据,将缓存数据回写到硬盘,以防数据丢失 */
            sync() ;
            reboot(RB_AUTOBOOT) ;
        }break;
    case 0x03: //心跳
        {
            ST_BYTE bvalue = pbuf[6];
            if(bvalue == 0x01)
                heart_beat();
            else if(bvalue == 0x02)
                return;
        }break;
    case 0x04: //覆盖文件
        {
            int res = coverFile();
            if(res){
                ST_BYTE buf[] = {0x0A,0x03,0x00,0x02,0x0B,0x04,0x02,0x20,0x16};
                this->Send(buf,sizeof(buf));  //回复管理机操作完成
            }
            else{
                ST_BYTE buf[] = {0x0A,0x03,0x00,0x02,0x0B,0x04,0x03,0x21,0x16};
                this->Send(buf,sizeof(buf));  //回复管理机操作完成
            }
        }break;
    case 0x05:{ //询问管理机名称
            send_devName();
        }break;
    case 0x07:{ //远程报文
            int PortMsg = pbuf[6]*256+pbuf[7];
            int res = m_manager->startMonitor(PortMsg);
            if(res)
            {
                ST_BYTE buf[] = {0x0A,0x03,0x00,0x02,0x0B,0x07,0x02,0x23,0x16};
                this->Send(buf,sizeof(buf));  //回复管理机操作完成
            }
            else
            {
                ST_BYTE buf[] = {0x0A,0x03,0x00,0x02,0x0B,0x07,0x03,0x24,0x16};
                this->Send(buf,sizeof(buf));  //回复管理机操作失败
            }

    }break;

    }

}


void   remoteProtocol::request_resend()
{
    ST_BYTE sendbuf[24];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x03;
    sendbuf[2] = 0x00;
    sendbuf[3] = 0x02;
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x03;
    sendbuf[6] = 0x03;
    ST_BYTE sum = 0x00;
    for(int i=0;i<7;i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[7] = sum;
    sendbuf[8] = 0x16;
    this->Send(sendbuf,9);
}

void   remoteProtocol::heart_beat()
{
    ST_BYTE sendbuf[24];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x03;
    sendbuf[2] = 0x00;
    sendbuf[3] = 0x02;
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x03;
    sendbuf[6] = 0x02;
    ST_BYTE sum = 0x00;
    for(int i=0;i<7;i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[7] = sum;
    sendbuf[8] = 0x16;
    this->Send(sendbuf,9);
}

void   remoteProtocol::send_devName()
{
    string devname = m_client->get_dev_name();
    string dnHex = sysTools::sstrToHex(devname);//sstrToHex(devname);
    //printf("devname : %s",devname.c_str());
    int byteLen = dnHex.length()/2;
    ST_BYTE *decBYTE = new ST_BYTE[byteLen];
    sysTools::str_to_hex((char *)dnHex.c_str(),decBYTE,dnHex.length());

    ST_BYTE sendbuf[1024];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x03;
    sendbuf[2] = ((byteLen+1)&0xFF00)>>8;
    sendbuf[3] = ((byteLen+1)&0x00FF);//byteLen;
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x06;             //命令码
    memcpy(&sendbuf[6],decBYTE,byteLen);

    ST_BYTE sum = 0x00;
    for(int i=0;i<(6+byteLen);i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[6+byteLen] = sum;
    sendbuf[7+byteLen] = 0x16;
    this->Send(sendbuf,8+byteLen);
}



void    remoteProtocol::send_fileSize(string fileName)
{

    string filePath = getfilePath(fileName);
    int fd = open(filePath.c_str(), O_RDWR);
	if (fd == -1)
	{
		printf("open [%s]  failed\n", filePath.c_str());
		return ;
	}
    else
        printf("Open file successful!\n");
    //计算文件大小
	int file_size = lseek(fd, 0, SEEK_END);
    printf("file size : %d KB\r\n",file_size);
    //m_fileID = fID;

    string dnHex = sysTools::sstrToHex(m_filename);//sstrToHex(devname);

    int byteLen = dnHex.length()/2;
    ST_BYTE *decBYTE = new ST_BYTE[byteLen];
    sysTools::str_to_hex((char *)dnHex.c_str(),decBYTE,dnHex.length());


    ST_BYTE sendbuf[64];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x02;
    int totalLen = byteLen + 5;//fileSize + standby
    sendbuf[2] = (totalLen&0xFF00)>>8;
    sendbuf[3] = (totalLen&0x00FF);
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x00;    //备用
    sendbuf[6] = file_size>>24;
    sendbuf[7] = file_size>>16;
    sendbuf[8] = file_size>>8;
    sendbuf[9] = file_size;

    memcpy(&sendbuf[10],decBYTE,byteLen);
    ST_BYTE sum = 0x00;
    for(int i=0;i<(10+byteLen);i++)
    {
        sum += sendbuf[i];
    }

    sendbuf[10+byteLen] = sum;
    sendbuf[11+byteLen] = 0x16;
    this->Send(sendbuf,12+byteLen);
}

bool    remoteProtocol::coverFile()
{
    DIR *dir;
    struct dirent *dirinfo;
    if ((dir = opendir("temp/")) == NULL){
        printf("dir is empty!");
        return FAILURE;
    }
    while ((dirinfo = readdir(dir)) != NULL)
    {
        if (strcmp(dirinfo->d_name, ".") == 0 || strcmp(dirinfo->d_name, "..") == 0)//判断是否是特殊目录
            continue;

        string tarPath = getfilePath(dirinfo->d_name);
        string srcPath = "temp/";
        srcPath.append(dirinfo->d_name);
        if(tarPath=="NULL")
        {
            char *s = strstr(dirinfo->d_name, ".so");     //检测是否是协议文件
            if(s==NULL){
                printf("can't find file in dir\r\n");
                return FAILURE;
            }
            else{
                tarPath = "/root/comm/protocols/";
                tarPath.append(dirinfo->d_name);
                int res = sysTools::copyFile(srcPath.c_str(),tarPath.c_str());
                if(!res)
                    return res;
            }
        }

        int res = sysTools::copyFile(srcPath.c_str(),tarPath.c_str());
        if(!res)
            return res;
    }
    return SUCCESS;
}













