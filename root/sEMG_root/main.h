/*
 * main.h: The header file of main.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#ifndef _MAIN_H
#define _MAIN_H

//#define ARM_VERSION // ARM版本
#define TEST_MODE

#define ROOT_VERSION	   0x0001 //程序版本号
#define RATE_1K            1  //采集频率 k
#define BRANCH_NUM	   8  //branch 数量
#define AD_CL_NUM          8  //每个branch的采集通道数
#define CHANNEL_NUM        ( BRANCH_NUM*AD_CL_NUM ) //总的采集通道数
#define MAX_CHANNEL_NUM	   128 //最大采集通道数

typedef enum {FALSE = 0,TRUE = 1} bool;

/*root struct definiton*/
struct root {
	unsigned int  version;
        unsigned char channel_num; //total channel num
	unsigned char AD_rate;	
};

/*branch struct definiton*/
struct branch{
        int   num;
        //bool  (*is_branch_ready)(void);
        bool  shake_hand;
        char  *device;
        unsigned char  *data_pool;	
        unsigned char  timeout;
};
/*init functions*/
bool root_init();
void branch_init();

#endif
