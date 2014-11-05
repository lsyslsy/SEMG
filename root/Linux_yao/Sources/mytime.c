/*
 * time.c: The time file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "../Headers/main.h"
#include <time.h>
#include "../Headers/mytime.h"
#include "../Headers/led.h"
#include <errno.h>
  
extern pthread_cond_t cond_tick;

void timeout_info(int signo)
{ 

	pthread_cond_broadcast(&cond_tick);//broadcast the condition

  // printf("the timer lost %d signals\n",k);
  //be sure to run in main thread
 //printf("the timer run in pid is %d\n",pthread_self());
//usleep(1);//加了也没用

    //printf("100ms time is up!\n");
}
 	 
/* init sigaction */
void init_sigaction(void)
{
     struct sigaction act;	 
     act.sa_handler = timeout_info;//指定信号处理海曙
     act.sa_flags   = 0;
     sigemptyset(&act.sa_mask);
     sigaction(SIGIO, &act, NULL);
}
 	 

 	
