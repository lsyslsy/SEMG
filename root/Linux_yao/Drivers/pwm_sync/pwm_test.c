#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "pwm_sync.h"
#include <unistd.h>

static void sig_ok(int signo)
{
	static unsigned long i = 0;
	printf("%ld\n",i++);
}

int main()
{
	int err;
	int fd = open("/dev/pwm_sync", O_RDONLY);
	err = signal(SIGIO, sig_ok);
	if(err == SIG_ERR) {
		perror("signal");
		return -1;
	}
	err = fcntl(fd, F_SETOWN, getpid());
	if(err < 0) {
		perror("set owner");
		return -1;
	}
	int oflags = fcntl(fd, F_GETFL);
	err = fcntl(fd, F_SETFL, oflags | FASYNC);
	if(err < 0) {
		perror("set async");
		return -1;
	}
	if(fd <0) {
		perror("open pwm_sync");
		return -1;
	}
	err += ioctl(fd, PWM_IOC_STOP,NULL);
	err += ioctl(fd, PWM_IOC_SET_FREQ, 10);
	err += ioctl(fd, PWM_IOC_START,NULL);
	if(err) {
		perror("ioctl");
		return -1;
	}
		while(1)
	pause();
	close(fd);
}