/*
 * time.h: The header file of time.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: Yao@2014.8.3
 */
#ifndef _MYTIME_H
#define _MYTIME_H

void init_sigaction(void);
void timeout_info(int signo);

#endif
