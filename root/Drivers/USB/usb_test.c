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
#include <sched.h>
#include <sys/ioctl.h>

#include "usb_semg.h"

#ifdef SEMG_ARM
#include "led.h"
#endif

#define TEST_TIME
#define TEST_SET_DELAY
#define TEST_DATA

static void pri_times(clock_t, struct timespec *, struct timespec *);
void parsedata(void *buffer);

int main(int argc, char const *argv[])
{
	int fd0, fd1;
	//int retval = 0;
	int i;
	ssize_t 		count = 0;
	unsigned char 			inbuffer[8192] = {0};
	char 			strbuf[20] ={0};
	struct timespec 		tpstart, tpend;
	clock_t 		start, end;
	struct sched_param param;
	int ret = 0;
	unsigned int j = 0;
	/*if (argc != 2) {
		printf("usage:%s <usbnum> \n", argv[0]);
		return -1;
	}*/
	//param.sched_priority = 10;
	// ret = sched_setscheduler(getpid(), SCHED_FIFO,&param);
 //    if(ret)
 //    {
 //        fprintf(stderr,"set scheduler failed \n");
 //        return -4;
 //    }

	sprintf(strbuf, "/dev/semg-usb%d", 1);
	fd0 = open(strbuf, O_RDONLY);
	if (fd0 < 0) {
		perror("open usb0 error");
		return -1;
	}
	// sprintf(strbuf, "/dev/semg-usb%d", 2);
	// fd1 = open(strbuf, O_RDONLY);
	// if (fd1 < 0) {
	// 	perror("open usb0 error");
	// 	return -1;
	// }
 	ret = ioctl(fd0, USB_SEMG_GET_BRANCH_NUM, NULL);
 	printf("ioctl get branchnum: %d\n", ret);

 	ret = ioctl(fd0, USB_SEMG_GET_CURRENT_FRAME_NUMBER, NULL);
	printf("ioctl get current framenumber: %d\n", ret);

 	ret = ioctl(fd0, USB_SEMG_GET_EXPECTED_FRAME_NUMBER, NULL);
	printf("ioctl get expected framenumber: %d\n", ret);
	ret = ioctl(fd0, USB_SEMG_SET_EXPECTED_FRAME_NUMBER, 900);
	printf("ioctl set expected framenumber: %d\n", ret);
	ret = ioctl(fd0, USB_SEMG_GET_EXPECTED_FRAME_NUMBER, NULL);
	printf("ioctl get expected framenumber: %d\n", ret);
	// TODO: a chajian trail end space
	for (j = 0; ; j++) {
		if ((start = clock_gettime(CLOCK_REALTIME, &tpstart)) == -1) {
			printf("times error");
			return -1;
		}
#ifdef SEMG_ARM
		Led_on(1);
#endif
#ifdef TEST_DATA
		count = read(fd0, inbuffer, 3258);
		if (count < 0) {
			fprintf(stderr, "read0 error code:%d; %s\n", errno, strerror(errno));
			//perror("read");
			return -1;
		}
#endif

#ifdef TEST_TIME
		if ((end = clock_gettime(CLOCK_REALTIME, &tpend)) == -1) {
				printf("times error");
				return -1;
		}
#endif

#ifdef SEMG_ARM
		Led_off(1);
#endif

#ifdef TEST_TIME
		pri_times(end - start, &tpstart, &tpend);
#endif

#ifdef TEST_DATA
		printf("count:%zd\n", count);
		parsedata(inbuffer);
#endif

	ret = ioctl(fd0, USB_SEMG_GET_CURRENT_FRAME_NUMBER, NULL);
	printf("ioctl get current framenumber: %d\n", ret);


#ifdef DEBUG_SET_DELAY
		if (j % 3 == 0) {
			ret = ioctl(fd0, USB_SEMG_SET_DELAY, 77);
 			printf("ioctl set delay: %d\n", ret);
		}
#endif
		usleep(100000);
		//printf("%zd:%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x\n", count, inbuffer[1019], inbuffer[1020], inbuffer[1021], inbuffer[1022],
		//	inbuffer[1023], inbuffer[1024], inbuffer[1025], inbuffer[1026]);
	}
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

void parsedata(void *buffer)
{
	int i =0;
	unsigned char *data = buffer;
	if (data[0] != 0xb7 || (data[2] << 8 | data[3]) != 3248)
		goto failed;
	printf("\033[40;34mbn\033[0m: %d, ", data[1]);
	printf("\033[40;34mfn\033[0m: %d, ", data[4] << 8 | data[5]);
	printf("\033[40;34mwait\033[0m: %d\n", data[6]);
	//return;
failed:
	printf("data error:\n");
	for (i = 0; i < 1000; i++) {
		printf("%#x,", data[i]);
				if (i  == 8 ) {
			printf("\n");
		}
	}
}