/*
 * time.h: The header file of time.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: Yao@2014.8.3
 */
#ifndef _MYTIME_H
#define _MYTIME_H

#define sEMG_tick_sec 	0 // sec
#define sEMG_tick_usec  100000  //usec

void init_sigaction(void);
void timeout_info(int signo);
int init_timer(void);

extern pthread_cond_t cond_tick;
extern pthread_mutex_t mutex_tick;
#endif