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

#define PI 3.1415926  //for simulate sin data
extern int Branch_Read_fd;
extern struct root root_dev;
extern struct branch branches[BRANCH_NUM];
extern unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE];

extern pthread_mutex_t mutex_buff;
extern pthread_cond_t cond_tick;
//#define SPI_MODE_1		(0|SPI_CPHA)
static unsigned char spi_recv_buf[BRANCH_NUM][BRANCH_BUF_SIZE]={0};
static unsigned char mode = SPI_MODE_1;//spi mode
static unsigned char bits = 8;
static unsigned int speed = 4156250;/*max clk:33250KHZ 256 division
 *5.5M = 33250000/5.5 = 6045456
 *4M = 33250000/8 = 4156250
 *3M = 33250000/11 = 3022728
 */

/**
 * Is_branch_ready: 判断branch是否将数据准备好
 * @num: branch号
 */

static bool Is_branch_ready(int num)
{
	char input;
	read(Branch_Read_fd, &input, 1);
	if ((~input >> num) & 0x01)
		return TRUE;//低电平返回TRUE
	else
		return FALSE;//高电平返回FALSE
}
static bool Is_branch_high(int num)
{
	char input;
	read(Branch_Read_fd, &input, 1);
	if ((~input >> num) & 0x01)
		return TRUE;//低电平返回TRUE
	else
		return FALSE;//高电平返回FALSE
}
static bool Is_branch_low(int num)
{
	char input;
	read(Branch_Read_fd, &input, 1);
	if ((~input >> num) & 0x01)
		return TRUE;//低电平返回TRUE
	else
		return FALSE;//高电平返回FALSE
}

/**
 * spi_init: initialize the SPI0 module
 *
 * Data length: 8-bit per unit
 * SPI mode: Master mode SPI_MODE_1
 * 下降沿采样，上升沿改变数据，平时SCLK=0(CPOL=0，CPHA=1)
 * Data shift order: MSB first
 *
 */

static int spi_init(int fd)
{
	int ret = 0;
	//spi mode
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		return ret;
	//bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		return ret;
	//max speed hz
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		return ret;
	return 0;
}

//!function forward declaration.
static int ParseDataPacket(unsigned char *p, int n);
static void print_data(unsigned char * pbuf, int branch_num);
static void BranchDataInit(unsigned char *recvbuf,int bn);
//test thread
void FunCollect()
{
}
/**
 *FunBranch: The branch thread entry function
 *@parameter: branch num(0-7)
 **/
void FunBranch(void* parameter)
{
	pthread_cleanup_push(pthread_mutex_unlock, &mutex_buff);
	struct branch *bx;
	unsigned char tx_buf[8] =
	{ 0 };
	unsigned char rx_buf[8] =
	{ 0 };
	unsigned char *pbuf;
	int branch_num, dev_spi, ret, size;
	unsigned long i;
	unsigned long tick = 0;//回绕是个问题, 用Jiffies是否更好
	//spi和数据处理的是否应该分开两个锁
	struct timespec slptm;
	branch_num = (int) parameter;

	bx = &branches[branch_num];
	
	dev_spi = open(bx->device, O_RDWR);
	if (dev_spi < 0)
	{
		DEBUG_ERROR("open spi%d failed!\n", branch_num);
		pthread_exit(NULL);
	}
	ret = spi_init(dev_spi);
	if (ret < 0)
	{
		DEBUG_ERROR("init spi%d failed!\n", branch_num);
		pthread_exit(NULL);
	}
	BranchDataInit(spi_recv_buf[branch_num], branch_num);
	int period = 100;
	unsigned int t = 0;
	pbuf = spi_recv_buf[branch_num];
	memcpy(data_pool[branch_num], pbuf, BRANCH_BUF_SIZE);
	while (1)
	{
		pthread_mutex_lock(&mutex_buff);
		pthread_cond_wait(&cond_tick, &mutex_buff);
		//相邻通道切换实测时间接近200 -500us   1ms以内
		//同一通道读写函数产生实际时钟间隔约100us左右
		//if (branch_num == 0 || branch_num == 7)
		//DebugInfo("branch%d thread is running!%ld\n", branch_num, tick++);
		
#ifdef ARM_VERSION 
		
		///想办法先MISO 再判断MOSI
		///或者先MOSI 再MISO
		//for(i = 0;i<2000000;i++);
		//i=100000   2ms
		//if(bx->has_shakehanded == FALSE)//确保双向通信正常
		unsigned int waittimes = 400;
		if(branch_num == 0)			/* only wait when branchnum=0 */
			while (!Is_branch_ready(branch_num) && waittimes > 0)
				waittimes--;
		size = read(dev_spi, pbuf, 3257);
		if(size < 0)//some error with dma
		{
			data_pool[branch_num][0] = 0x48;//spi you gui le
			goto spi_error;
		}
		int tmp = ParseDataPacket(pbuf, branch_num);
		if (tmp == 0)
		{
			print_data(pbuf, branch_num);
			//Data verified, then copy to socket
			memcpy(data_pool[branch_num], pbuf, BRANCH_BUF_SIZE);
		}
		else
		{
			/***********************tmp*****************/
			data_pool[branch_num][0] = 0xee; //data error
			DebugWarn("Data Packet from Branch%d have wrong bytes:%d\n",
					branch_num, tmp);
			printf("read:%d,%x,%x,%x,%x,%x\n", size, pbuf[0], pbuf[1], pbuf[2],
					pbuf[3], pbuf[3256]);
		}
		//printf("Buffer ID: %x\n", pbuf[1999]);

		//usleep(1);
#else
		pbuf = bx->data_pool;
		*pbuf = 0xb7;
		*(pbuf + 1) = branch_num;
		*(pbuf + 2) = (BRANCH_DATA_SIZE >> 8);
		*(pbuf + 3) = (unsigned char) BRANCH_DATA_SIZE;
		pbuf += 8;
		moni_data(pbuf, CHANNEL_NUM_OF_BRANCH , branch_num, &t, &period);
#endif

		spi_error: pthread_mutex_unlock(&mutex_buff);
	}//while(1)


	close(dev_spi);

	pthread_cleanup_pop(0);
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
	p += 8;
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
/**
 * Parse the data packet from branch.
 * 验证数据包格式是否正确.
 * @param p 传入数据包地址指针.
 * @param n 数据包大小.
 * @return 帧格式中错误的字节数
 */
static int ParseDataPacket(unsigned char *p, int n)
{
	int i, j;
	int count = 0;
	if (p[0] != 0xb7)
		count++;
	if (p[1] != n)
		count++;
	if (p[2] != (BRANCH_DATA_SIZE >> 8))
		count++;
	if (p[3] != (unsigned char) BRANCH_DATA_SIZE)
		count++;
	p += 8;
	for (i = 0; i < CHANNEL_NUM_OF_BRANCH; i++)
	{
		if (*p != 0x11)
			count++;
		p++;
		if (*p != i + n * CHANNEL_NUM_OF_BRANCH)
			count++;
		p++;
		p++;//skip the state
		for (j = 0; j < 200; j++)
		{
			//	if(*p != j) count++;
			p++;
		}
	}
	if (*p != 0xED)
		count++;
	return count;
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

static void print_data(unsigned char * pbuf, int branch_num)
{
	unsigned char buf_lbl = pbuf[6] & 0x01U;
	unsigned char buf_status = pbuf[6] >> 1 & 0x07U;
	unsigned char buf_ov = pbuf[7];

	char ch_buf_lbl[6];
	switch (buf_lbl)
	{
	case 0:
		strcpy(ch_buf_lbl, "Left ");
		break;
	case 1:
		strcpy(ch_buf_lbl, "Right");
		break;
	}

	char ch_status[9];
	switch (buf_status)
	{
	case -1:
		break;
	case 0:
		strcpy(ch_status, "Empty");
		break;
	case 1:
		break;
	case 2:
		strcpy(ch_status, "Write");
		break;
	case 3:
		strcpy(ch_status, "Full    ");
		break;
	case 4:
		strcpy(ch_status, "Overflow");
		break;
	default:
		break;
	}
	DebugInfo("Branch%d Data Right. Buf %s. Status: %s. OV: %d. \n",
			branch_num, ch_buf_lbl, ch_status, buf_ov, pbuf[6]);
}
