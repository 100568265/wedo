/* Example of CAN for PISO-CAN200/400 Cards
 *
 *    Author: Golden Wang
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* File level history (record changes for this file here.)
 *
 *    v 0.0.0 25 Mar 2010 by Golden Wang
 *         create, blah blah...
*/

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

#define ip_cmd_set_can1_params "ip link set can1 type can bitrate 125000"
#define ip_cmd_can1_up "ifconfig can1 up"
#define ip_cmd_can1_down "ifconfig can1 down"
//#include "can.h"
//#include "raw.h"

#define PF_CAN 29



extern int optind, opterr, optopt;

static int skt = -1;
static int running = 1;

void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [can-interface]\n", prg);
}

void sigterm(int signo)
{
	printf("got signal %d\n", signo);
	running = 0;
}

int main(int argc, char **argv)
{
    system(ip_cmd_set_can1_params);//设置参数
    system(ip_cmd_can1_up);//打开can1接口
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
	int opt;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
	struct can_filter rfilter;
	int nbytes, i;
	int verbose = 0;

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);

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

	/* only receive the CAN frame of CAN ID 0x123 */
//	rfilter.can_id   = 0x123;
//	rfilter.can_mask = CAN_SFF_MASK;

	/* receive all CAN frame */

	rfilter.can_id   = 0x123;
    rfilter.can_mask = 0x123;

	setsockopt(skt, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	addr.can_family = family;
	strcpy(ifr.ifr_name, argv[optind]);
	ioctl(skt, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(skt, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	printf("\nReceive CAN message from interface %s\n", ifr.ifr_name);

	while (running)
	{
		if ((nbytes = read(skt, &frame, sizeof(frame))) < 0) {
			perror("read");
			return 1;
		}

		if (frame.can_id & CAN_RTR_FLAG)
		{
			printf("remote request");
		}
		else
		{
			printf("Receive CAN Message ID : 0x%x Length : %d\n", frame.can_id, frame.can_dlc);
	                for(i = 0; i < frame.can_dlc; i++)
        	        {
                	        printf("Data[%d] : %02X\n",i ,frame.data[i]);
                	}

	                printf("\n");
		}
	}

	return 0;
}
