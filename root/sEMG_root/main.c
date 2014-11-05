/*
 * main.c: The main file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include "main.h"
#include "time.h"
#include "socket.h"
#include "collect.h"
#include "led.h"

struct root root_dev;
struct branch branches[BRANCH_NUM];
unsigned char data_pool[BRANCH_NUM][2000] = {0}; // branch data pool
int branch_priority[BRANCH_NUM] = {9,8,7,6,5,4,3,2};//越大优先级越高

pthread_mutex_t mutex_buff;
pthread_cond_t cond_tick;

#ifdef ARM_VERSION
int GPIO_L8_fd = -1;
int Branch_Read_fd = -1;
#endif

//init the root status
bool root_init()
{
    root_dev.version = ROOT_VERSION;
    root_dev.channel_num = CHANNEL_NUM;
    root_dev.AD_rate = RATE_1K;
#ifdef ARM_VERSION
    GPIO_L8_fd = open("/dev/GPIO_L8", O_RDWR);  // 打开数据中断脚 L8
    if (GPIO_L8_fd < 0) {
        printf("Can't open /dev/GPIO_L8\n");
        return FALSE;
    }
    Branch_Read_fd = open("/dev/sEMG_Input", O_RDWR);  // 打开标志输入脚0～7
    if (Branch_Read_fd  < 0) {
        printf("Can't open /dev/sEMG_Input\n");
        return FALSE;
    }
#endif
    pthread_mutex_init(&mutex_buff,NULL);
    pthread_cond_init(&cond_tick,NULL); 
    return TRUE;
}//root_init()
//init the branches' status
void branch_init()
{
   int i;
   branches[0].num = 0;
   branches[0].shake_hand = FALSE;
   //branches[0].is_branch_ready = is_b0_ready;
   branches[0].device = "/dev/spidev0.0";
   branches[0].data_pool= data_pool[0];
   branches[0].timeout = 0;
   
   branches[1].num = 1;
   branches[1].shake_hand = FALSE;
   //branches[1].is_branch_ready = is_b1_ready;
   branches[1].device = "/dev/spidev0.1";
   branches[1].data_pool= data_pool[1];
   branches[1].timeout = 0;
   
   branches[2].num = 2;
   branches[2].shake_hand = FALSE;
   //branches[2].is_branch_ready = is_b2_ready;
   branches[2].device = "/dev/spidev0.2";
   branches[2].data_pool= data_pool[2];
   branches[2].timeout = 0;
   
   branches[3].num = 3;
   branches[3].shake_hand = FALSE;
   //branches[3].is_branch_ready = is_b3_ready;
   branches[3].device = "/dev/spidev0.3";
   branches[3].data_pool= data_pool[3];
   branches[3].timeout = 0;
   
   branches[4].num = 4;
   branches[4].shake_hand = FALSE;
   //branches[4].is_branch_ready = is_b4_ready;
   branches[4].device = "/dev/spidev0.4";
   branches[4].data_pool= data_pool[4];
   branches[4].timeout = 0;
   
   branches[5].num = 5;
   branches[5].shake_hand = FALSE;
   //branches[5].is_branch_ready = is_b5_ready;
   branches[5].device = "/dev/spidev0.5";
   branches[5].data_pool= data_pool[5];
   branches[5].timeout = 0;

   branches[6].num = 6;
   branches[6].shake_hand = FALSE;
   //branches[6].is_branch_ready = is_b6_ready;
   branches[6].device = "/dev/spidev0.6";
   branches[6].data_pool= data_pool[6];
   branches[6].timeout = 0;

   branches[7].num = 7;
   branches[7].shake_hand = FALSE;
   //branches[7].is_branch_ready = is_b7_ready;
   branches[7].device = "/dev/spidev0.7";
   branches[7].data_pool= data_pool[7];
   branches[7].timeout = 0;
 
}//branch_init()
/**
  *main() funciton: contain 9 pthreads(1 socket send thread && 
  *               8 branch collect threads), branch collect 
  *               threads have higher priority. All the pthreads 
  *      	  are triggered by the timer(100ms per).
**/
int main()
{

    bool ret;
    int i;
    i=getuid();
    if(i==0)
	printf("The current user is root\n");
    else {
	printf("The current user is not root\n");
	return 0;
    }
    ret = root_init();
    if(!ret){
        printf("root_init error!\n");
        return 0;
    }  
    branch_init();

    struct sched_param param;
    pthread_t p_socket;
    pthread_attr_t attr_socket;
    pthread_t p_branch[BRANCH_NUM];
    pthread_attr_t attr_branch[BRANCH_NUM];
    
    pthread_attr_init(&attr_socket);
    for(i=0;i<BRANCH_NUM;i++)
         pthread_attr_init(&attr_branch[i]);

    param.sched_priority = 1 ;
    pthread_attr_setschedpolicy(&attr_socket,SCHED_RR);
    pthread_attr_setschedparam(&attr_socket,&param);
    pthread_attr_setinheritsched(&attr_socket,PTHREAD_EXPLICIT_SCHED);//do not inherit father's attr

    for(i=0;i<BRANCH_NUM;i++){
         pthread_attr_init(&attr_branch[i]);
         param.sched_priority= branch_priority[i];
         pthread_attr_setschedpolicy(&attr_branch[i],SCHED_RR);
         pthread_attr_setschedparam(&attr_branch[i],&param);
         pthread_attr_setinheritsched(&attr_branch[i],PTHREAD_EXPLICIT_SCHED);
         //pthread_create(&p_socket,&attr_socket,(void *)FunBranch,(void *)i);
	 pthread_create(&p_branch[i],&attr_branch[i],(void *)FunBranch,(void *)i);
    }
    pthread_create(&p_socket,&attr_socket,(void *)FunSocket,NULL);
  
   init_sigaction();//init timer
   init_time(); 
    printf("app thread is running!\n");
 
   //uninit  
    for(i=0;i<BRANCH_NUM;i++){
        pthread_join(p_branch[i],NULL);
        pthread_attr_destroy(&attr_branch[i]);
    }
    pthread_join(p_socket,NULL);
    pthread_attr_destroy(&attr_socket);

    pthread_mutex_destroy(&mutex_buff);
#ifdef ARM_VERSION
    close(GPIO_L8_fd);
    close(Branch_Read_fd);
#endif

    return 0;
}//main()







