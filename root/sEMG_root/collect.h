/*
 * collect.h: The header file of collect.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#ifndef _COLLECT_H
#define _COLLECT_H

void Fun_hander(void *arg);
void FunCollect();
void FunBranch(void* parameter);

void fsin_to_char(float f, unsigned char *buf);
void moni_data(unsigned char *pbuf, int channel_num, int branch_num);
#endif
