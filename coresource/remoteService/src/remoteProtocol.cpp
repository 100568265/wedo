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
#include "Manager.h"


remoteProtocol::remoteProtocol(TcpClientPort *client):
    m_client(client)
{
    //ctor
}

remoteProtocol::~remoteProtocol()
{
    //dtor
}

template<class T>
int length(T& arr)
{
    //cout << sizeof(arr[0]) << endl;
    //cout << sizeof(arr) << endl;
    return sizeof(arr) / sizeof(arr[0]);
}

string getfilePath(int FID)
{
    string path;
    switch(FID){
    case 1:
        {
            path = "/root/comm/config/comm_network.xml";
        }break;
    case 2:
        {
            path = "/root/comm/config/comm_configparam.xml";
        }break;
    case 3:
        {
            path = "etc/comm/variabletransformer.xml";
        }break;
    case 4:
        {
            path = "etc/comm/transformerprotocoltype.xml";
        }break;
    case 5:
        {
            path = "etc/comm/template.xml";
        }break;
    case 6:
        {
            path = "etc/comm/channel.xml";
        }break;
    case 7:
        {
            path = "/root/comm/log.log";
        }break;
    default:
        break;
    }
    return path;
}

string getTmpfilePath(int FID)
{
    string path;
    switch(FID){
    case 1:
        {
            path = "temp/comm_network.xml";
        }break;
    case 2:
        {
            path = "temp/comm_configparam.xml";
        }break;
    case 3:
        {
            path = "temp/variabletransformer.xml";
        }break;
    case 4:
        {
            path = "temp/transformerprotocoltype.xml";
        }break;
    case 5:
        {
            path = "temp/template.xml";
        }break;
    case 6:
        {
            path = "temp/channel.xml";
        }break;
    case 7:
        {
            path = "temp/log.log";
        }break;
    default:
        break;
    }
    return path;
}


int    remoteProtocol::onRead(ST_BYTE *pbuf,int& readed)
{
    if(sizeof(pbuf)<4)
        return -1;
    if(pbuf[0]==0x0A&&pbuf[4]==0x0B)
    {
        int pufLen = pbuf[3]+7;

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
/*    if(code == 0x03)
    {
        heart_beat();
    }*/
    return true;

}

void    remoteProtocol::parsing_01(ST_BYTE *pbuf)
{
    ST_BYTE funType = pbuf[2];
    switch(funType){
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
        {
            send_fileSize(funType);
        }break;
    case 0x08:
        {
            send_devName();
        }
    }

}

void    remoteProtocol::parsing_02(ST_BYTE *pbuf)
{
    int fileSize = pbuf[5]*256*256*256 + pbuf[6]*256*256 + pbuf[7]*256 +pbuf[8];
    if(fileSize<=0)
        return;

    bool delFSign = (pbuf[2]>>7)?1:0;  //最高位用来做是否清空缓存的标志位
    if(delFSign)
        sysTools::DeleteFile("temp/");

    switch(pbuf[2]&0x7F){
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
            {
                //int fileSize = pbuf[5]*256*256*256 + pbuf[6]*256*256 + pbuf[7]*256 +pbuf[8];
                if(fileSize>0)
                {
                    ST_BYTE buf1[] = {0x0A,0x03,0x01,0x01,0x0B,0x01,0x1B,0x16};//0A 03 01 01 0B 01 CS 16
                    this->Send(buf1,sizeof(buf1));  //回复准备接收

                    //sleep(0.5);
                    ST_BYTE funType = pbuf[2];      //File ID
                    int res = m_client->rece_File(getTmpfilePath(funType).c_str(),fileSize);  //接收文件
                    if(res)
                    {
                        ST_BYTE buf2[] = {0x0A,0x03,0x01,0x01,0x0B,0x02,0x1C,0x16};//0A 03 01 01 0B 02 CS 16
                        this->Send(buf2,sizeof(buf2));  //回复接收成功
                    }
                    else
                    {
                        ST_BYTE buf3[] = {0x0A,0x03,0x01,0x01,0x0B,0x03,0x1D,0x16};//0A 03 01 01 0B 02 CS 16
                        this->Send(buf3,sizeof(buf3));  //回复接收失败
                    }
                }
            }break;
        case 0x09:
            {
                int datalen  = pbuf[3];
                //int fileSize = pbuf[5]*256*256*256 + pbuf[6]*256*256 + pbuf[7]*256 +pbuf[8];
                int fileNameSize = datalen - 4;
                char *soName = new char[fileNameSize+1];
                memcpy(soName,&pbuf[9],fileNameSize);      //hex 2 char
                soName[fileNameSize] = '\0';

                ST_CHAR fullname[256] = {0};
                strcpy(fullname,"temp/");
                strcat(fullname,soName);

                printf("filePath :%s\r\n",fullname);

                ST_BYTE buf1[] = {0x0A,0x03,0x01,0x01,0x0B,0x01,0x1B,0x16};//0A 03 01 01 0B 01 CS 16
                this->Send(buf1,sizeof(buf1));  //回复准备接收

                int res = m_client->rece_File((const char*)fullname,fileSize);  //接收文件
                if(res)
                {
                    ST_BYTE buf2[] = {0x0A,0x03,0x01,0x01,0x0B,0x02,0x1C,0x16};//0A 03 01 01 0B 02 CS 16
                    this->Send(buf2,sizeof(buf2));  //回复接收成功
                }
                else
                {
                    ST_BYTE buf3[] = {0x0A,0x03,0x01,0x01,0x0B,0x03,0x1D,0x16};//0A 03 01 01 0B 02 CS 16
                    this->Send(buf3,sizeof(buf3));  //回复接收失败
                }
                delete[] soName;
            }break;
        default:
            break;
    }


}

//0A 01 05 00 0b
void    remoteProtocol::parsing_03(ST_BYTE *pbuf)
{
    ST_BYTE funType = pbuf[2];
    switch(funType){
    case 0x01:
        {
            ST_BYTE bvalue = pbuf[5];
            if(bvalue == 0x01){
                m_client->send_File(getfilePath(m_fileID).c_str());
            }

        }break;
    case 0x02:
        {
            ST_BYTE buf2[] = {0x0A,0x03,0x02,0x01,0x0B,0x02,0x1C,0x16};//0A 03 01 01 0B 02 CS 16
            this->Send(buf2,sizeof(buf2));  //回复接收成功
            /* 同步磁盘数据,将缓存数据回写到硬盘,以防数据丢失 */
            sync() ;
            reboot(RB_AUTOBOOT) ;
        }break;
    case 0x03:
        {
            ST_BYTE bvalue = pbuf[5];
            if(bvalue == 0x01)
                heart_beat();
            else if(bvalue == 0x02)
                return;
        }break;
    case 0x04:
        {
            string tagPath;
            if(pbuf[5] == 0x01){
                tagPath = "temp2/";//"/root/comm/config/";
            }
            else if(pbuf[5] == 0x02){
                tagPath = "temp2/";//"/etc/comm/";
            }
            bool res = sysTools::copyDir("temp/",tagPath.c_str());
            if(res){
                ST_BYTE buf1[] = {0x0A,0x03,0x04,0x01,0x0B,0x02,0x1F,0x16};//0A 03 01 01 0B 01 CS 16
                this->Send(buf1,sizeof(buf1));  //覆盖原文件成功
            }
            else{
                ST_BYTE buf2[] = {0x0A,0x03,0x04,0x01,0x0B,0x03,0x20,0x16};//0A 03 01 01 0B 01 CS 16
                this->Send(buf2,sizeof(buf2));  //覆盖原文件失败
            }

            //todo: 覆盖完文件后删除temp文件内的文件
            //sysTools::DeleteFile("temp/");
        }

    }

}


void   remoteProtocol::request_resend()
{
    ST_BYTE sendbuf[24];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x03;
    sendbuf[2] = 0x01;
    sendbuf[3] = 0x01;
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x03;
    ST_BYTE sum = 0x00;
    for(int i=0;i<6;i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[6] = sum;
    sendbuf[7] = 0x16;
    this->Send(sendbuf,8);
}

void   remoteProtocol::heart_beat()
{
    ST_BYTE sendbuf[24];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x03;
    sendbuf[2] = 0x03;
    sendbuf[3] = 0x01;
    sendbuf[4] = 0x0B;
    sendbuf[5] = 0x02;
    ST_BYTE sum = 0x00;
    for(int i=0;i<6;i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[6] = sum;
    sendbuf[7] = 0x16;
    this->Send(sendbuf,8);
}

void   remoteProtocol::send_devName()
{
    string devname = m_client->get_dev_name();
    string dnHex = sysTools::sstrToHex(devname);//sstrToHex(devname);

    int byteLen = dnHex.length()/2;
    ST_BYTE *decBYTE = new ST_BYTE[byteLen];
    sysTools::str_to_hex((char *)dnHex.c_str(),decBYTE,dnHex.length());

    ST_BYTE sendbuf[1024];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x02;
    sendbuf[2] = 0x08;
    sendbuf[3] = byteLen;//byteLen;
    sendbuf[4] = 0x0B;
    memcpy(&sendbuf[5],decBYTE,byteLen);

    ST_BYTE sum = 0x00;
    for(int i=0;i<(5+byteLen);i++)
    {
        sum += sendbuf[i];
    }
    sendbuf[5+byteLen] = sum;
    sendbuf[6+byteLen] = 0x16;
    this->Send(sendbuf,7+byteLen);
}



void    remoteProtocol::send_fileSize(int fID)
{
    string file_path;
    file_path = getfilePath(fID);



    int fd = open(file_path.c_str(), O_RDWR);
	if (fd == -1)
	{
		printf("open [%s]  failed\n", file_path.c_str());
		return ;
	}
    else
        printf("Open file successful!\n");
    //计算文件大小
	int file_size = lseek(fd, 0, SEEK_END);

    printf("file size : %d KB\r\n",file_size);
    m_fileID = fID;

    ST_BYTE sendbuf[24];
    sendbuf[0] = 0x0A;
    sendbuf[1] = 0x02;
    sendbuf[2] = 0x05;
    sendbuf[3] = 0x04;
    sendbuf[4] = 0x0B;

    sendbuf[5] = file_size>>24;
    sendbuf[6] = file_size>>16;
    sendbuf[7] = file_size>>8;
    sendbuf[8] = file_size;

    ST_BYTE sum = 0x00;
    for(int i=0;i<9;i++)
    {
        sum += sendbuf[i];
    }

    sendbuf[9] = sum;
    sendbuf[10] = 0x16;
    this->Send(sendbuf,11);
}




















