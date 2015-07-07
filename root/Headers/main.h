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
#define BRANCH_NUM         12  //branch 数量

// SEMG related
#define SEMG_NUM            8 // semg 数量
#define CHANNEL_NUM_OF_SEMG         16  //每个branch的采集通道数
//#define CHANNEL_USED_NUM        ( BRANCH_USED_NUM*CHANNEL_NUM_OF_BRANCH  ) //总的采集通道数
#define CHANNEL_NUM        ( SEMG_NUM*CHANNEL_NUM_OF_SEMG  ) //总的采集通道数
#define MAX_CHANNEL_NUM         128 //最大采集通道数
#define CHANNEL_BUF_SIZE          203 //1byte(0x01)+1byte(channel number)+1byte(state)+200bytes(100ms data 2*100)
#define SEMG_DATA_SIZE         ( CHANNEL_BUF_SIZE*CHANNEL_NUM_OF_SEMG )
#define SEMG_HEADER_SIZE      9
#define SEMG_TAIL_SIZE      1
#define SEMG_FRAME_SIZE              (SEMG_HEADER_SIZE + SEMG_DATA_SIZE + SEMG_TAIL_SIZE)


// Motion Sensor related
#define SENSOR_NUM          4 // sensor 数量
#define SENSOR_DATA_SIZE    92 // 0x12, branchnum, 100HZ数据
#define SENSOR_HEADER_SIZE      9
#define SENSOR_TAIL_SIZE      1
#define SENSOR_FRAME_SIZE              (SENSOR_HEADER_SIZE + SENSOR_DATA_SIZE + SENSOR_TAIL_SIZE)


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
        int type; // 1: SEMG, 2: SENSOR
        bool  has_shakehanded;
        bool  need_shakehand;
        int devfd;
        unsigned char  *data_pool;
        int size;
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

#endif