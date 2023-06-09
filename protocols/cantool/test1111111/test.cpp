#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
using namespace std;

int main()
{
	system("ifconfig can0 down");
	system("ip link set can0 type can bitrate 250000");
	system("ifconfig can0 up");
	return 0;
}
