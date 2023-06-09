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
int readLen;
int num;

int main()
{
    can_fd = socket(PF_CAN,SOCK_RAW,CAN_RAW);   //create socket
    strcpy(ifr.ifr_name,"can0");
    ioctl(can_fd,SIOCGIFINDEX,&ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_fd, (struct sockaddr *)&addr, sizeof(addr));


    while(1)
    {
        readLen = read(can_fd,&frame,sizeof(frame));
        if(readLen >0)
        {
            for(int i=0;i<frame.can_dlc;i++)
            {
                std::cout << frame.data[i]<< " ";
            }
        }
    }
return 0;
}
