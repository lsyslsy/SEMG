/*
 * time.h: The header file of time.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#ifndef _TIME_H
#define _TIME_H

#define sEMG_tick_sec 	0 // sec
#define sEMG_tick_usec  100000  //usec

void init_time(void);
void init_sigaction(void);
void timeout_info(int signo);

#endif
