/*
 * collect.c: The collect file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "main.h"
#include "collect.h"
#include "led.h" 
#include "math.h"
#include <errno.h>

void thread_test(void* parameter)
{


    int policy;
	struct sched_param param;
	printf("thread test start\n");
	pthread_getschedparam(pthread_self(), &policy,
                         &param);
	printf("the testthread pid is %d\n",pthread_self());
	printf("thread test's priority:%d\n",param. __sched_priority);
   while(1)  
   {
   sleep(1);
  
   //printf("the testthread pid is %d\n",pthread_self());
   }
   //usleep(1);//printf("thread test's priority:%d\n",param. __sched_priority);;

}