#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#define ip_cmd_set_can0_params "ip link set can0 type can bitrate 250000"
#define ip_cmd_can0_up "ifconfig can0 up"
#define ip_cmd_can0_down "ifconfig can0 down"

//#include "can.h"

#define PF_CAN 29


extern int optind, opterr, optopt;

static int skt = -1;


void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [can-interface]\n", prg);
}

void Send(sendbuf, len)
{

}

int main(int argc, char **argv)
{
    system(ip_cmd_set_can0_params);//设置参数
    system(ip_cmd_can0_up);//打开can0接口


	int family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
	int opt;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
	int nbytes, i;
	int verbose = 0;

	if (optind == argc) {
		print_usage(basename(argv[0]));
		exit(0);
	}

	printf("interface = %s, family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW\n", argv[optind]);

	if ((skt = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strcpy(ifr.ifr_name, argv[optind]);
	ioctl(skt, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(skt, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}


	do
        {
		printf("\nPress 'Enter' to send data from interface %s\n", ifr.ifr_name);
                getchar();

		frame.can_id = 0x123;
                frame.data[0] = 0x01;
                frame.data[1] = 0x02;
                frame.data[2] = 0x03;
                frame.data[3] = 0x04;
                frame.data[4] = 0x05;
                frame.data[5] = 0x06;
                frame.data[6] = 0x07;
                frame.data[7] = 0x08;
                frame.can_dlc = 8;
                write(skt, &frame, sizeof(frame));

                printf("Send CAN Message ID : 0x%x Length : %d\n", frame.can_id, frame.can_dlc);
		for(i = 0; i < frame.can_dlc; i++)
		{
			printf("Data[%d] : %02X\n",i ,frame.data[i]);
		}

		printf("\nPress 'Esc' to quit or Press 'Enter' to run again\n");

        }while((getchar() != 27));

	return 0;
}
