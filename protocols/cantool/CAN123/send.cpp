#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>



struct sockaddr_can addr;
struct ifreq ifr;
struct can_frame frame;
int can_fd;
int address;
int sendLen;
int num;

//prototypes
int SendYC();
int SendYX();
int SendSOE();
int SendFG();
int SendDL();
int SendDevice();
void printFrame();


int main()
{
    system("ifconfig can0 down");
    system("ip link set can0 type can bitrate 250000");
    system("ifconfig can0 up");


    can_fd = socket(PF_CAN,SOCK_RAW,CAN_RAW);   //create socket
    strcpy(ifr.ifr_name,"can0");
    ioctl(can_fd,SIOCGIFINDEX,&ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_fd, (struct sockaddr *)&addr, sizeof(addr));
    std::cout << "Please enter the device address: ";
    std::cin >> address;
    while(1)
    {
    std::cout << "Enter a number to send data: (1-6), Enter -1 to quit:\n";
    std::cin >> num;
    if(num ==-1)
    {
        exit;
    }
    switch(num)
    {
        case 1 :
        SendYC();
        break;
        case 2 :
        SendYX();
        break;
        case 3 :
        SendSOE();
        break;
        case 4 :
        SendFG();
        break;
        case 5 :
        SendDL();
        break;
        case 6 :
        SendDevice();
        break;
    }
    }





}


void printFrame()
{
    std::cout <<frame.can_id;
    std::cout <<frame.can_dlc;
    for(int i=0;i<frame.can_dlc;i++)
    {
        std::cout << frame.data[i] << std::endl;
    }
    std::cout << std::endl;
}

int SendYC()    // 01 01 56H
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 1;
    frame.data[0] = 0x56;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}

int SendYX()    //01 01 59H
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 1;
    frame.data[0] = 0x59;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}

int SendSOE()   //01 02 40H 00
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 2;
    frame.data[0] = 0x40;
    frame.data[1] = 0x00;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}

int SendFG()    //01 01 52H
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 1;
    frame.data[0] = 0x52;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}

int SendDL()    //01 02 58H
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 1;
    frame.data[0] = 0x58;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}

int SendDevice()    //01 01 63H
{
    frame.can_id = (address<<3)|5;
    frame.can_dlc = 1;
    frame.data[0] = 0x63;
    sendLen = write(can_fd,&frame,sizeof(frame));
    std::cout << "sendLen = " <<sendLen<<std::endl;
    printFrame();
    return sendLen;
}



