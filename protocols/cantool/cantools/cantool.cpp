#include <linux/can.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include <thread>
#include <condition_variable>
#include <mutex>



struct sockaddr_can addr;
struct ifreq ifr;
struct can_frame frame;
struct can_frame status;
int can_fd;
int address;
int sendLen;
int readLen;
int num;

// prototypes
int SendYC();
int SendYX();
int SendSOE();
int SendFG();
int SendDL();
int SendDevice();

// receive function
void receive()
{
    while(1)
    {
        readLen = read(can_fd, &status, sizeof(status));
    std::cout << "readLen = " << readLen << ", status.can_id = " << status.can_id << ", status.can_dlc = " << status.can_dlc << ", sizeof(status) = " << sizeof(status) << std::endl;
    }
}

int main()
{

    system("ifconfig can0 down");
    system("ip link set can0 type can bitrate 250000");
    system("ifconfig can0 up");


    // create socket can_fd
    int can_fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);

    if (can_fd < 0)
    {
        std::cout <<"socket can create error!\n";
        return -1;
    }
    // bind device can0 with socket
    strcpy(ifr.ifr_name, "can0");
    ioctl(can_fd, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;           // protocol family
    addr.can_ifindex = ifr.ifr_ifindex; // device index
    // bind socket with can0
    int bind_res = bind(can_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (bind_res < 0)
    {
        std::cout <<"bind error!";
        return -1;
    }

    // send
    std::cout << "Please enter the device address: ";
    std::cin >> address;

    std::thread mythread(receive);
    mythread.detach();
    while (1)
    {
        std::cout << "Enter a number to send data: (1-6), Enter -1 to quit";
        std::cin >> num;
        if (num == -1)
        {
            exit;
        }
        switch (num)
        {
        case 1:
            SendYC();
            break;
        case 2:
            SendYX();
            break;
        case 3:
            SendSOE();
            break;
        case 4:
            SendFG();
            break;
        case 5:
            SendDL();
            break;
        case 6:
            SendDevice();
            break;
        }
    }

    return 0;
}

int SendSOE() // 01 02 40H 00
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 2;
    frame.data[0] = 0x40;
    frame.data[1] = 0x00;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

int SendYC() // 01 01 56H
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 1;
    frame.data[0] = 0x56;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

int SendYX() // 01 01 59H
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 1;
    frame.data[0] = 0x59;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

int SendFG() // 01 01 52H
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 1;
    frame.data[0] = 0x52;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

int SendDL() // 01 02 58H
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 1;
    frame.data[0] = 0x58;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

int SendDevice() // 01 01 63H
{
    frame.can_id = (address << 3) | 5;
    frame.can_dlc = 1;
    frame.data[0] = 0x63;
    sendLen = write(can_fd, &frame, sizeof(frame));
    std::cout << "sendLen = " << sendLen << std::endl;
    return sendLen;
}

