#include "TcpClientPort.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
 #include <fcntl.h>
#include <unistd.h>
#include "syslogger.h"

TcpClientPort::TcpClientPort(const char *addr,int port,const char * devname)
{
    //ctor
    memset(m_ipaddr,0,sizeof(m_ipaddr));
    memcpy(m_ipaddr,addr,strlen(addr));
/*    string str = devname;
    printf("str :%s\n",str.c_str());
    memcpy(m_devname,str.c_str(),str.length());*/
    m_devname = devname;
    m_port = port;

    printf("addr : %s,port: %d,devname : %s\n",addr,port,devname);
    printf("m_ipaddr : %s,port: %d,devname : %s\n",m_ipaddr,m_port,m_devname.c_str());
    m_sockfd = -1;
    memset(&remote_addr,0,sizeof(remote_addr));

}

TcpClientPort::~TcpClientPort()
{
    //dtor
}

string    TcpClientPort::get_dev_name()
{
    //memcpy(dName,m_devname,strlen(m_devname));
    string ss(this->m_devname);
    return ss;
}


int     TcpClientPort::Open()
{
    if(m_isopen)
        return 1;
    remote_addr.sin_family=AF_INET; //设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr(m_ipaddr);//服务器IP地址
	remote_addr.sin_port=htons(m_port); //服务器端口号

	/*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((m_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error");
		return -1;
	}

    SetKeepalive(m_sockfd, 1, 180, 60, 20);
	/*将套接字绑定到服务器的网络地址上*/
	if(connect(m_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect error");
		return -1;
	}

    m_isopen = true;
	printf("connected to server\n");
	SysLogger::GetInstance()->LogInfo("connected to server: %s\n",m_ipaddr);
	return 1;
}

bool  TcpClientPort::isOpen()
{
    return m_isopen;
}

void TcpClientPort::Close()
{
    if(m_isopen){
        printf("%s client is close !\n",m_devname.c_str());
        SysLogger::GetInstance()->LogWarn("%s client is close !\n",m_devname.c_str());
        m_isopen = false;
        close(m_sockfd);
    }
}


char *  TcpClientPort::get_Ipaddr()
{
    return m_ipaddr;
}

void TcpClientPort::SetKeepalive(ST_INT socket_fd, ST_INT kpalive,ST_INT kpidle,ST_INT kpintval,ST_INT kpcout)
{
    ST_INT keepalive = kpalive;//1;
    ST_INT keepidle = kpidle;//30;
    ST_INT keepinterval = kpintval;//5;
    ST_INT keepcount =kpcout;// 3;
    setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE,  (void*)&keepalive ,    sizeof(keepalive));     // 开启keepalive属性
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPIDLE,  (void*)&keepidle ,     sizeof(keepidle));      // 如该连接在60秒内没有任何数据往来,则进行探测
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPINTVL, (void*)&keepinterval , sizeof(keepinterval));  // 探测时发包的时间间隔为5 秒
    setsockopt(socket_fd, SOL_TCP,    TCP_KEEPCNT,   (void*)&keepcount ,    sizeof(keepcount));     // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
}

int TcpClientPort::recv_Buf(unsigned char *bufs,int &datalen)
{
    if(!m_isopen)
    {
       Open();//this->conn_Server();
       return 0;
    }
    FD_ZERO  (&m_ReadSet);             //清空合集
    FD_SET   (m_sockfd, &m_ReadSet);

    struct timeval iv = { 3, 0 };
    int ret = select(m_sockfd + 1, &m_ReadSet, NULL, NULL, &iv);
    if(!ret) return 0;

    if (ret < 0)
    {
        printf("tcpclient select error: %d,desc: %s", errno, strerror(errno));
        return 0;
    }

    if(! FD_ISSET(m_sockfd, &m_ReadSet))
        return 0;
    //char chars[2048];
    int readlen = recv(m_sockfd, bufs, 1024, 0);
    if (readlen > 0) {
        //printf("%s\n",chars);
        datalen = readlen;
        return 1;
    }
    else if(readlen < 0 && (EAGAIN == errno || EINTR == errno || EWOULDBLOCK == errno))
    {
        return 0;
    }
    else if(readlen<0&&(errno == ETIMEDOUT))
    {
        this->Close();
        printf("keeplive time out!\n");
        SysLogger::GetInstance()->LogError("Close socket!keeplive time out!desc :%s",strerror(errno));
    }
    else {
        this->Close();
        if (readlen)
            printf("tcpclient recv error: %d,desc: %s", errno, strerror(errno));
            SysLogger::GetInstance()->LogError("tcpclient recv error: %d,desc: %s", errno, strerror(errno));
    }
    return 0;
}

int  TcpClientPort::send_Buf(ST_BYTE *sendbuf,int buflen)
{
    if(!m_isopen)
    {
        this->Open();
        return false;
    }
    ST_INT lastlen = buflen, sendlen = 0;
    try{

    while (lastlen > 0) {
        sendlen = send(m_sockfd, sendbuf, lastlen, 0);
        if(sendlen >= 0) {
            sendbuf     += sendlen;
            lastlen -= sendlen;
            continue;
        }

        int err_ = errno;
        switch (err_) {
            case EAGAIN:        case EINTR://         case EWOULDBLOCK:
                sleep(0.1);
                continue;
            case EMSGSIZE:      case ENOBUFS:
                return false;

//            case EPIPE:         case ECONNRESET:
            default:
                this->Close();
            // logger  ...;
                return false;
        }
    }
    }
    catch(...){
        printf("catch error 6!\n");
    }
    return true;
}


int   TcpClientPort::send_File(const char *fileName)
{
    int fd = open(fileName, O_RDWR);
	if (fd == -1)
	{
		printf("open [%s]  failed\n", fileName);
		return -1;
	}
    else
        printf("Open file successful!\n");

    char buf[1024] = {0};
    //文件光标偏移到文件开始位置
	lseek(fd, 0, SEEK_SET);

    int send_len = 0;//记录发送了多少字节
    int ret = -1;
    while (1)
	{
		bzero(buf, sizeof(buf));
		//读取数据
		ret = read(fd, buf, sizeof(buf));
		if (ret <= 0)
		{
			printf("send file[%s] succeed!!!!\n", fileName);
			break;
		}

		//发送数据
		write(m_sockfd, buf, ret);

		send_len += ret;//统计发送了多少字节
	}
	// 关闭文件
	close(fd);
    return 1;
}

int   TcpClientPort::rece_File(const char *fileName,int fileSize)
{
    //创建新文件
    int fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0666);

    int size = fileSize;    //atoi(file_len);//文件大小
    int write_len = 0;      //记录已写入的字节数
    char buf[1024] = {0};   //数据缓存
    int ret = -1;

    while(1)
    {
        bzero(buf, 1024);

        //接收文件数据
        ret = read(m_sockfd, buf, sizeof(buf));
        if( ret <= 0)
        {
            printf("\n [recv-%s] receive file done!!!\n", fileName);
            break;
        }

        //将数据写入文件
        write(fd, buf, ret);

        write_len += ret;//记录写入的字节数

        //动态的输出接收进度
        printf("uploading %.2f%% \n", (float)write_len/size * 100);
        if(write_len == size)
            break;
    }

    // 关闭文件
	close(fd);

    if(size != write_len)
    {
        printf("File Size is not match");
        return -1;
    }


    return 1;
}






