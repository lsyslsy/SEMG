/*
 * main.h: The header file of main.c for root board of the sEMG project
 *
 *      All right reserved.
 *
 *      Created: HY@2013.5.10
 */
#ifndef _MAIN_H
#define _MAIN_H

#define ROOT_VERSION       0x0001 //程序版本号
#define RATE_1K            1  //采集频率 k
#define BRANCH_NUM         8  //branch 数量
#define CHANNEL_NUM_OF_BRANCH          16  //每个branch的采集通道数
//#define CHANNEL_USED_NUM        ( BRANCH_USED_NUM*CHANNEL_NUM_OF_BRANCH  ) //总的采集通道数
#define CHANNEL_NUM        ( BRANCH_NUM*CHANNEL_NUM_OF_BRANCH  ) //总的采集通道数
#define MAX_CHANNEL_NUM         128 //最大采集通道数
#define CHANNEL_BUF_SIZE          203 //1byte(0x01)+1byte(channel number)+1byte(state)+200bytes(100ms data 2*100)
#define BRANCH_DATA_SIZE         ( CHANNEL_BUF_SIZE*CHANNEL_NUM_OF_BRANCH )
#define BRANCH_BUF_SIZE                 3600
#define BRANCH_Header_SIZE      9
#define BRANCH_Tail_SIZE      1
//用于和ARM通信的命令字
#define CMD_SHKEHAND          1
#define CMD_TRANSFER          2

//ARM上发的状态字
#define STATE_OK              1
typedef enum {FALSE = 0,TRUE = 1} bool;

/*root struct definiton*/
struct root {
        unsigned int  version;
        unsigned char channel_num; //total channel num
        unsigned char AD_rate;
        unsigned char connected_branches_count;
};

/*branch struct definiton*/
struct branch{
        int   num;
        //bool  (*is_branch_ready)(void);
        bool  has_shakehanded;
        bool  need_shakehand;
        int devfd;
        unsigned char  *data_pool;
        unsigned char  timeout;
        bool is_connected;
        unsigned int expected_fn;       // expected frame number
        unsigned int waitms;
};
/*init functions*/
int root_init();
void branch_init();
void thread_test(void* parameter);

extern struct branch branches[BRANCH_NUM];
extern struct root root_dev;
extern unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE];

#endif