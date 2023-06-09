
#include "CommManager.h"
#include "sysmutex.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <signal.h>

#include <termios.h>
#define PORTCOM "/dev/ttyS"

using namespace std;

#define LOCKFILE "./txjmain.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int lockfile (int fd)
{
    struct flock f1;
    f1.l_type  = F_WRLCK;
    f1.l_start = 0;
    f1.l_whence= SEEK_SET;
    f1.l_len   = 0;
    return (fcntl(fd, F_SETLK, &f1));
}

int alreadyRunning (void)
{
    int fd;
    char buf[16];
    fd = open (LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if(fd < 0) {
        printf("txjmain is already running...\n");
        exit(1);
    }
    if(lockfile(fd) < 0) {
        if(errno == EACCES || errno == EAGAIN) {
             close(fd);
            return (1);
        }
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return (0);
}

int main (int argc, char *argv[])
{
    fprintf (stdout, "=== Communication Management Main Server [Version %s] ===\n\n", TXJ_VERSION_STR);

    if(alreadyRunning()) {
        printf("txjmain is running................\n");
        return (0);
    }
    else {
        printf("txjmain begin running.............\n");
    }
    Mutex mutex;

/*    ST_INT ret;
    ST_INT com_fd;
    com_fd = open("/dev/ttyS7",O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(com_fd == -1) {
    }
    fcntl(com_fd, F_SETFL, 0);
    isatty(STDIN_FILENO);*/
//    ret=Set_Speed(com_fd,9600);
//    ret=Set_Parity(com_fd,8,1,0);

    printf("Create Comm Server................\n");
    CCommManager commManager;

//	cmmManager->ConnectDatabase();

    printf("Load All Configs..................\n");
	if(commManager.LoadConfigs()) return -2;

	commManager.Start();
	printf("Comm Server is running............\n");

    mutex.Wait(-1);
	return 0;
}
