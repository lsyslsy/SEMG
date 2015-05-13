/*
 * Led.h: The header file of Led.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.9.10
 */
#ifndef _LED_H
#define _LED_H

#define LED_ON    0
#define LED_OFF   1

#define LED_1      0
#define LED_2      1
#define LED_3      2
#define LED_4      3

int Led_on(int Led_num);
int Led_off(int Led_num);




#endif

