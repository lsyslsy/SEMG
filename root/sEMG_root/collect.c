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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "main.h"
#include "collect.h"
#include "led.h"
#include "math.h"

#define PI 3.1415926  //for simulate sin data

extern int Branch_Read_fd;
extern struct root root_dev;
extern struct branch branches[BRANCH_NUM];
extern unsigned char data_pool[BRANCH_NUM][2000];

extern pthread_mutex_t mutex_buff;
extern pthread_cond_t cond_tick;

static unsigned char mode = SPI_MODE_1;//spi mode
static unsigned char bits = 8;
static unsigned int  speed = 6045456;/*max clk:33250KHZ 256 division
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
	      return TRUE;
	else
	      return FALSE;
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
static int Spi_init(int fd)
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
void Fun_hander(void *arg)
{
     free(arg);
     (void)pthread_mutex_unlock(&mutex_buff);
}

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
     pthread_cleanup_push(Fun_hander, &mutex_buff);
     struct branch *bx;
     unsigned char tx_buf[8];
     unsigned char *pbuf;
     int num,dev_spi,ret,i,size;
     unsigned long tick = 0;

     num = (int)parameter;
     bx = &branches[num];
#ifdef ARM_VERSION
     dev_spi = open(bx->device, O_RDWR);
     if (dev_spi < 0){
         printf("open spi%d failed!\n",num);
         return;
     }
     ret = Spi_init(dev_spi);
     if (ret < 0){
         printf("init spi%d failed!\n",num);
         return;
     }	
#endif	
     while(1){
         pthread_mutex_lock(&mutex_buff);
         pthread_cond_wait(&cond_tick,&mutex_buff);
         if (num == 0 || num == 7)
     	     printf("branch%d thread is running!%ld\n",num,tick++);
      //   Led_on(num);
#ifdef ARM_VERSION 
         if(bx->shake_hand == FALSE){        
         	tx_buf[0] = 0xa1;
		tx_buf[1] = num;
		write(dev_spi,tx_buf,1);
		while(!Is_branch_ready(num));
		read(dev_spi,pbuf,2);
		if(*pbuf!=0xa2 || *(pbuf+1)!=num)
			printf("spi data error!\n");
		else
			bx->shake_hand == TRUE;
         }
         else{
		//tx_buf[0] = 0xa3;
		//tx_buf[1] = num;
		//write(dev_spi,tx_buf,2);
		while(!Is_branch_ready(num));
                printf("get low power!\n");
                //size = read(dev_spi,pbuf,10);
	        //printf("read:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",size,pbuf[0],pbuf[1],pbuf[2],pbuf[3],pbuf[4],pbuf[5],pbuf[6],pbuf[7],pbuf[8],pbuf[9]);
		size = read(dev_spi,pbuf,2000);
	        printf("read:%d,%x,%x,%x,%x,%x\n",size,pbuf[0],pbuf[1],pbuf[2],pbuf[1998],pbuf[1999]); 	       
		//usleep(1);
         }
#else
         pbuf = bx->data_pool;
         *pbuf = 0xa4;
         *(pbuf+1)=num;
         *(pbuf+2)=0x06;
         *(pbuf+3)=0x50;
         pbuf+=4;  
         moni_data(pbuf, AD_CL_NUM, num);
#endif 
       pthread_mutex_unlock(&mutex_buff);
      }//while(1)

#ifdef ARM_VERSION
      close(dev_spi);
#endif
      pthread_cleanup_pop(0);
}//FunBranch()


//test function: produce sin data
void fsin_to_char(float f, unsigned char *buf)
{
    float f_sin;
    f_sin = 1 + f;
    *buf = (char)(f_sin*100);
    *(buf+1) = (char)((f_sin*10000) - (int)(f_sin*100)*100);
}

//test function: simulate the branch data
void moni_data(unsigned char *pbuf, int channel_num, int branch_num)
{
   int i,j;
   unsigned char buf[2];
   if(test_count == 0)
   {
   for (j = 0;j<channel_num;j++)
   {         
	*pbuf = 0x10;
        *(pbuf+1) = j + branch_num*channel_num;
        if(j%2)
        {
            for (i=0;i<100;i++){
            // *(pbuf + 2 + i) = tbuf[i%20];   
                fsin_to_char(sin(i*PI/50), buf);         
	        *(pbuf + 2 + i*2) = buf[0];
	        *(pbuf + 2 + i*2 +1) = buf[1]; 
              }
        }
        else
	{
            for (i=0;i<100;i++){
                *(pbuf + 2 + i*2) = 0x64;
	        *(pbuf + 2 + i*2 +1) = 0x00;
              }
        }
        pbuf += 202; 
   }
   test_count = 1;
   }
   else
   {
   for (j = 0;j<channel_num;j++)
   {         
	*pbuf = 0x10;
        *(pbuf+1) = j + branch_num*channel_num;
        if(j%2)
        {
            for (i=0;i<100;i++){
            // *(pbuf + 2 + i) = tbuf[i%20];   
                fsin_to_char(sin(i*PI/100), buf);         
	        *(pbuf + 2 + i*2) = buf[0];
	        *(pbuf + 2 + i*2 +1) = buf[1]; 
              }
        }
        else
	{
            for (i=0;i<100;i++){
                *(pbuf + 2 + i*2) = 0x00;
	        *(pbuf + 2 + i*2 +1) = 0x00;
              }
        }
        pbuf += 202; 
   }
   test_count = 0;
   }
}



