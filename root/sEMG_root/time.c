/*
 * time.c: The time file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "main.h"
#include "time.h"
#include "led.h"	 

extern pthread_cond_t cond_tick;

extern int GPIO_L8_fd;

void timeout_info(int signo)
{ 
   pthread_cond_broadcast(&cond_tick);//broadcast the condition
#ifdef ARM_VERSION
    ioctl(GPIO_L8_fd,0,0); //每周期取反，中断 
#endif
    //printf("100ms time is up!\n");
}
 	 
/* init sigaction */
void init_sigaction(void)
{
     struct sigaction act;	 
     act.sa_handler = timeout_info;
     act.sa_flags   = 0;
     sigemptyset(&act.sa_mask);
     sigaction(SIGALRM, &act, NULL);
}
 	 
/* init */
void init_time(void)
{
     struct itimerval val;	 
     val.it_value.tv_sec = sEMG_tick_sec;  //0
     val.it_value.tv_usec = sEMG_tick_usec;  //100ms
     val.it_interval = val.it_value;
     setitimer(ITIMER_REAL, &val, NULL);//timer start
}

 	
