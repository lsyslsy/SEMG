//
//用于测试usb驱动
//
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>


static void pri_times(clock_t, struct timespec *, struct timespec *);


int main(int argc, char const *argv[])
{
	int fd0, fd1;
	//int retval = 0;
	int i;
	ssize_t 		count = 0;
	char 			inbuffer[8192] = {0};
	char 			strbuf[20] ={0};
	struct timespec 		tpstart, tpend;
	clock_t 		start, end;
	/*if (argc != 2) {
		printf("usage:%s <usbnum> \n", argv[0]);
		return -1;
	}*/

	sprintf(strbuf, "/dev/semg-usb%d", 0);
	fd0 = open(strbuf, O_RDONLY);
	if (fd0 < 0) {
		perror("open usb0 error");
		return -1;
	}
	sprintf(strbuf, "/dev/semg-usb%d", 1);
	fd1 = open(strbuf, O_RDONLY);
	if (fd1 < 0) {
		perror("open usb0 error");
		return -1;
	}

	if ((start = clock_gettime(CLOCK_REALTIME, &tpstart)) == -1) {
		printf("times error");
		return -1;
	}

	count = read(fd0, inbuffer, 3257);
	if (count < 0) {
		fprintf(stderr, "error code:%d; %s\n", errno, strerror(errno));
		//perror("read");
		return -1;
	}
	//count = read(fd1, inbuffer, 3257);
	if (count < 0) {
		fprintf(stderr, "error code:%d; %s\n", errno, strerror(errno));
		//perror("read");
		return -1;
	}

	if ((end = clock_gettime(CLOCK_REALTIME, &tpend)) == -1) {
		printf("times error");
		return -1;
	}
	pri_times(end - start, &tpstart, &tpend);
	printf("count:%zd", count);
	for (i = 0; i < 10; i++) {
		if ((i % 10) ==0 ) printf("\n");
		printf("%#x,", inbuffer[i]);
	}
	printf("\n");
	//printf("%zd:%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x\n", count, inbuffer[1019], inbuffer[1020], inbuffer[1021], inbuffer[1022], 
	//	inbuffer[1023], inbuffer[1024], inbuffer[1025], inbuffer[1026]);

	return 0;
}

static void pri_times(clock_t real, struct timespec *tpstart, struct timespec *tpend) {
	long		timediff = 0;

	// if (clktck == 0)	/* fetch clock ticks per second first time */
	// 	if ((clktck = sysconf(_SC_CLK_TCK)) < 0) {
	// 		perror("sysconf error");
	// 		exit(1);
	// 	}
	timediff = (tpend->tv_sec - tpstart->tv_sec) * 1000000000 + (tpend->tv_nsec - tpstart->tv_nsec);
	printf("elapsed time:%fs\n", timediff / (double) 1000000000);
	// printf("  real:  %7.6f\n", real / (double) clktck);
	// printf("  user:  %7.6f\n",
	//   (tpend->tms_utime - tpstart->tms_utime) / (double) clktck);
	// printf("  sys:   %7.6f\n",
	//   (tpend->tms_stime - tpstart->tms_stime) / (double) clktck);
	// printf("  child user:  %7.6f\n",
	//   (tpend->tms_cutime - tpstart->tms_cutime) / (double) clktck);
	// printf("  child sys:   %7.6f\n",
	//   (tpend->tms_cstime - tpstart->tms_cstime) / (double) clktck);
}