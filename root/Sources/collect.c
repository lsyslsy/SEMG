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

#include "../Headers/main.h"
#include "../Headers/collect.h"
#include "../Headers/led.h"
#include "math.h"
#include <errno.h>
#include "../Headers/semg_debug.h"
#include "../Headers/process.h"

#define PI 3.1415926  //for simulate sin data

extern struct root root_dev;
extern struct branch branches[BRANCH_NUM];
extern unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE];

extern pthread_mutex_t mutex_tick;
extern pthread_cond_t cond_tick;
extern int capture_state; // 0:start, 1:processing, 2:finish
extern struct work_queue semg_queue;

unsigned char spi_recv_buf[BRANCH_NUM][BRANCH_BUF_SIZE]={{0}};



//!function forward declaration.

static void BranchDataInit(unsigned char *recvbuf,int bn);

/**
 *FunBranch: The branch thread entry function
 *@parameter: branch num(0-7)
 **/
void FunBranch(void* parameter)
{
	struct branch *bx;
	unsigned char *pbuf;
	int size;
	int branch_num;
	unsigned long tick = 0;//回绕是个问题, 用Jiffies是否更好
	//TODO spi和数据处理的是否应该分开两个锁
	//struct timespec slptm;


	// dev_spi = open(bx->device, O_RDON);
	// if (dev_spi < 0) {
	// 	DebugError("open usb%d failed!\n", branch_num);
	// 	pthread_exit(NULL);
	// }
	for (branch_num = 0; branch_num < BRANCH_NUM; branch_num++) {
		BranchDataInit(spi_recv_buf[branch_num], branch_num);
		pbuf = spi_recv_buf[branch_num];
		bx = &branches[branch_num];
		memcpy(bx->data_pool, pbuf, BRANCH_BUF_SIZE); // 格式化data_poll中的数据
	}

	while (1) {
		// wait for period interrupt
		pthread_mutex_lock(&mutex_tick);
		while (capture_state != 0) // wait for start
			pthread_cond_wait(&cond_tick, &mutex_tick);
		capture_state = 1; // mark state as processing
		pthread_mutex_unlock(&mutex_tick);
		// SEMG process

		//相邻通道切换实测时间接近200 -500us   1ms以内
			//同一通道读写函数产生实际时钟间隔约100us左右
			// if (branch_num == 0 || branch_num == 7)
			// DebugInfo("branch%d thread is running!%ld\n", branch_num, tick++);
		for (branch_num = 0; branch_num < BRANCH_NUM; branch_num++) {
			bx = &branches[branch_num];
			pbuf = spi_recv_buf[branch_num];
			if (bx->is_connected == FALSE)
				continue;

#ifndef MONI_DATA
			size = read(bx->devfd, pbuf, 3258);
			if(size != 3258) { // any unresolved error
				bx->data_pool[0] = 0x48;//spi you gui le
				DebugError("read semg%d failed(ErrCode %d): %s\n", branch_num, errno, strerror(errno));
				bx->is_connected = FALSE;
				close(bx->devfd);
				bx->devfd = -1;
				continue;
			}
			// DebugInfo("read branch%d 3258 bytes\n", branch_num);
			queue_put(&semg_queue, 1, branch_num);
#else
			int period = 100;
			pbuf = bx->data_pool;
			*pbuf = 0xb7;
			*(pbuf + 1) = branch_num;
			*(pbuf + 2) = (BRANCH_DATA_SIZE >> 8);
			*(pbuf + 3) = (unsigned char) BRANCH_DATA_SIZE;
			pbuf += 9;
			moni_data(pbuf, CHANNEL_NUM_OF_BRANCH , branch_num, &t, &period);
#endif
			// send message to processer
			// 通过邮箱容量比如为2或3，来判断是否满确定处理是否来得及
		}

		// motion sensor process

		//
		pthread_mutex_lock(&mutex_tick);
		// attention: 有可能在处理完成前又发生中断了，表明处理来不及处理时,这时会被hanlder设成0:start
		if (capture_state == 1) // when proceesing
			capture_state = 2; // mark finish
		pthread_mutex_unlock(&mutex_tick);

	}


}//FunBranch()

//!ddd
static void BranchDataInit(unsigned char *recvbuf,int bn)
{
	int i, j;
	unsigned char *p = recvbuf;
	p[0] = 0xb7;
	p[1] = (char)bn;
	p[2] = BRANCH_DATA_SIZE>>8;
	p[3] = (unsigned char)BRANCH_DATA_SIZE ;
	p += 9;
	for (i = 0; i < CHANNEL_NUM_OF_BRANCH; i++)
	{
		*p = 0x11;
		p++;
		*p = i + bn * CHANNEL_NUM_OF_BRANCH;
		p++;
		p++;//skip the state
		for (j = 0; j < 200; j++)
		{
			p++;
		}
	}
	*p = 0xED;


}

//!test function: produce sin data.
void fsin_to_char(float f, unsigned char *buf)
{
	*buf =(char)(((short int )(f*32767))>>8);
	*(buf + 1) = (char) ((short int )(f*32767));

	//*buf = (char) (f_sin * 100);
	//*(buf + 1) = (char) ((f_sin * 10000) - (int) (f_sin * 100) * 100);
}

//!test function: simulate the branch data
void moni_data(unsigned char *pbuf, int channel_num, int branch_num, unsigned int *test_count, int * period)
{
	int i, j;
	unsigned char buf[2];
	for (j = 0; j < channel_num; j++)
	{
		*pbuf = 0x11;
		*(pbuf + 1) = j + branch_num * channel_num;
		//if ( j == 10 || j==9)
		//if (j % 2)
		{
			for (i = 0; i < 100; i++)
			{
				// *(pbuf + 2 + i) = tbuf[i%20];
				fsin_to_char(i*1.0/ 100,buf);//sin(2*i * PI / *period), buf);
				*(pbuf + 2 + i * 2) = buf[1];
				*(pbuf + 2 + i * 2 + 1) = buf[0];
			}
		}
		pbuf += 203;
	}
	*test_count = 1;

	*pbuf = 0xed;
	if(*period == 20)
		*period = 100;
	else
		*period = *period - 1;

}