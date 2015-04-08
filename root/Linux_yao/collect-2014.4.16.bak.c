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
#include "semg_debug.h"

#define PI 3.1415926  //for simulate sin data
extern int Branch_Read_fd;
extern struct root root_dev;
extern struct branch branches[BRANCH_NUM];
extern unsigned char data_pool[BRANCH_NUM][3600];

extern pthread_mutex_t mutex_buff;
extern pthread_cond_t cond_tick;
//#define SPI_MODE_1		(0|SPI_CPHA)
static unsigned char mode = SPI_MODE_1;//spi mode
static unsigned char bits = 8;
static unsigned int speed = 4156250;/*max clk:33250KHZ 256 division
 *5.5M = 33250000/5.5 = 6045456
 *4M = 33250000/8 = 4156250
 *3M = 33250000/11 = 3022728
 */
unsigned int test_count = 0;//test mode
/**
 * Is_branch_ready: 判断branch是否将数据准备好
 * @num: branch号
 */
#ifdef ARM_VERSION
static bool Is_branch_ready(int num)
{
	char input;
	read(Branch_Read_fd,&input,1);
	if((~input>>num) & 0x01)
	return TRUE;//低电平返回TRUE
	else
	return FALSE;//高电平返回FALSE
}
static bool Is_branch_high(int num)
{
	char input;
	read(Branch_Read_fd,&input,1);
	if((~input>>num) & 0x01)
	return TRUE;//低电平返回TRUE
	else
	return FALSE;//高电平返回FALSE
}
static bool Is_branch_low(int num)
{
	char input;
	read(Branch_Read_fd,&input,1);
	if((~input>>num) & 0x01)
	return TRUE;//低电平返回TRUE
	else
	return FALSE;//高电平返回FALSE
}
#endif	
/**
 * spi_init: initialize the SPI0 module
 *
 * Data length: 8-bit per unit
 * SPI mode: Master mode SPI_MODE_1
 * 下降沿采样，上升沿改变数据，平时SCLK=0(CPOL=0，CPHA=1)
 * Data shift order: MSB first
 *
 */
#ifdef ARM_VERSION
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
}
#endif

static int ParseDataPacket(unsigned char *p,int n);
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
	unsigned char tx_buf[8]={0};
	unsigned char rx_buf[8]={0};
	unsigned char *pbuf;
	int branch_num, dev_spi, ret,  size;
	unsigned long i;
	unsigned long tick = 0;
    struct timespec slptm;
	branch_num = (int) parameter;
	bx = &branches[branch_num];
	

#ifdef ARM_VERSION
	dev_spi = open(bx->device, O_RDWR);
	if (dev_spi < 0)
	{
		DEBUG_ERROR("open spi%d failed!\n",branch_num);
		pthread_exit(NULL);
	}
	ret = spi_init(dev_spi);
	if (ret < 0)
	{
		DEBUG_ERROR("init spi%d failed!\n",branch_num);
		pthread_exit(NULL);
	}
#endif	
    pbuf = bx->data_pool;
   for(i = 0; i < 2000; i++)
    {
        pbuf[i] = 0xABU;
       
    }

   //while(1) ;//sleep(2);
   //while(!Is_branch_ready(0));

      slptm.tv_sec = 0;
       slptm.tv_nsec = 10000;      //1000 ns = 1 us
	 //  printf("the branch%d thread pid is %d\n",branch_num, pthread_self());
	  // while(1) usleep(1);
	   /*int policy;
	struct sched_param param;
	pthread_getschedparam(pthread_self(), &policy,
                         &param);
	printf("Current thread's priority:%d\n",param. __sched_priority);*/
	while (1)
	{
		pthread_mutex_lock(&mutex_buff);
		pthread_cond_wait(&cond_tick, &mutex_buff);
		//相邻通道切换实测时间接近200 -500us   1ms以内
		//同一通道读写函数产生实际时钟间隔约100us左右
		//if (branch_num == 0 || branch_num == 7)
		DebugInfo("branch%d thread is running!%ld\n", branch_num, tick++);
#ifdef ARM_VERSION 
		pbuf = bx->data_pool;
		///想办法先MISO 再判断MOSI
		///或者先MOSI 再MISO
	   //for(i = 0;i<2000000;i++);
	   //i=100000   2ms
	   bx->has_shakehanded = TRUE; //Skip the shakehand 
		if(bx->has_shakehanded == FALSE)//确保双向通信正常
		{
			//线程可能阻塞在这,由于其他线程无法获得互斥锁，程序将死在这。
			while(!Is_branch_ready(branch_num));
			tx_buf[0] = 0xa5;
			tx_buf[1] = branch_num;
			tx_buf[2] = CMD_SHKEHAND;
			tx_buf[3] = 136;
			//MOSI
			if(write(dev_spi,tx_buf,4)<0)
			{
				DebugError("spi write failed when shake hand\n");
				goto spi_error;
			}
			
			//usleep(1);
			//MISO
			//读写之间实测较接近100us
			rx_buf[0]=0;
			rx_buf[1]=0;
			rx_buf[2]=0;
			if(read(dev_spi,rx_buf,3)<0)
			{
				DebugError("spi read failed when shake hand\n ");
				goto spi_error;
			}
			//printf("[0]=%d,[1]=%d,[2]=%d\n",rx_buf[0],rx_buf[1],rx_buf[2]);
			if(*rx_buf!=0xb6 )
			{
			
				DebugWarn("shake hand with Branch%d failed!\n", branch_num);
				DebugWarn("[0]=%d,[1]=%d,[2]=%d\n",rx_buf[0],rx_buf[1],rx_buf[2]);
				goto spi_error;
			}
			else if(*(rx_buf+1)!=branch_num)
			{
				DebugError("Branch%d Connected  wrong !\n", branch_num);
				goto spi_error;
			}
			else   
			{
				bx->has_shakehanded = TRUE;
				DebugInfo("shake hand  with branch%d success!\n",branch_num);
			}
		}
		else
		{
		   /* bx->has_shakehanded = FALSE;

			while(!Is_branch_ready(branch_num));

			tx_buf[0] = 0xa5;
			tx_buf[1] = branch_num;
			tx_buf[2] = CMD_TRANSFER;
			//MOSI
			if(write(dev_spi,tx_buf,4)<0)
			{
				DebugError("spi write failed when shake hand\n");
				goto spi_error;
			}*/
			//读写之间实测较接近100us
			//usleep(1);
			//printf("get low power!\n");
			//size = read(dev_spi,pbuf,10);
			//printf("read:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",size,pbuf[0],pbuf[1],pbuf[2],pbuf[3],pbuf[4],pbuf[5],pbuf[6],pbuf[7],pbuf[8],pbuf[9]);
	    int lednum = branch_num <4 ? branch_num :4;
	  // Led_on(lednum);
		unsigned int waittimes = 400;
			while(!Is_branch_ready(branch_num)&&waittimes>0) 
			waittimes--;
			//Led_off(lednum);
		size = read(dev_spi,pbuf,3257);
			int tmp=ParseDataPacket(pbuf,branch_num);
			if(tmp==0 )
			{
				unsigned char buf_lbl = pbuf[6] & 0x01U;
			    unsigned char buf_status = pbuf[6] >> 1 & 0x07U;
			    unsigned char buf_ov = pbuf[7];
                
                char ch_buf_lbl[6];
                switch(buf_lbl)
                {
                case 0:
                    strcpy(ch_buf_lbl, "Left ");
                    break;
                case 1:
                    strcpy(ch_buf_lbl, "Right");
                    break;
                }

                char ch_status[9];
                switch(buf_status)
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
				DebugInfo("Branch%d Data Right. Buf %s. Status: %s. OV: %d. \n",branch_num, ch_buf_lbl, ch_status, buf_ov, pbuf[6]);
			
				//DebugInfo("Data Packet Received from Branch%d ALL Right\n",branch_num);
				
			}
			else
			{
				DebugWarn("Data Packet from Branch%d have wrong bytes:%d\n",branch_num, tmp);
				printf("read:%d,%x,%x,%x,%x,%x\n",size,pbuf[0],pbuf[1],pbuf[2],pbuf[3],pbuf[3256]);
			}
			//printf("Buffer ID: %x\n", pbuf[1999]);
			
			//usleep(1);
		} 
#else
		pbuf = bx->data_pool;
		*pbuf = 0xa4;
		*(pbuf + 1) = branch_num;
		*(pbuf + 2) = 0x06;
		*(pbuf + 3) = 0x50;
		pbuf += 4;
		moni_data(pbuf, CHANNEL_NUM_OF_BRANCH , branch_num);
#endif
	/*	*/

		spi_error:
		pthread_mutex_unlock(&mutex_buff);
	}//while(1)

#ifdef ARM_VERSION
	close(dev_spi);
#endif
	pthread_cleanup_pop(0);
}//FunBranch()

///

static int ParseDataPacket(unsigned char *p,int n)
{
	   int i,j;
	   int count = 0;
	    if(p[0]  != 0xb7) count++;
	    if(p[1]  != n) count++;
	    if(p[2]  != (BRANCH_DATA_SIZE>>8)) count++;
	    if(p[3]  != (unsigned char)BRANCH_DATA_SIZE) count++;
	    p += 8;
	    for(i=0;i<16;i++)
	    {
	    	if(*p != 0x11) {
			count++;printf("h");}
	    	p++;
	    	if(*p != i + n*CHANNEL_NUM_OF_BRANCH) {count++;printf("f");}
	        p++;
			p++;//skip the state 
	    	for(j = 0;j<200;j++)
	    	{
	    	//	if(*p != j) count++;
	    		p++;
	    	}
	    }
	    if(*p != 0xED) count++;
	    return count;
}


