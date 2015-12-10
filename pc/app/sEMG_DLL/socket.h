#ifndef _SOCKET_H_
#define _SOCKET_H_

#ifdef IN_WINDOWS
#include <windows.h>
#endif

#include <mutex>


/*************** constant *****************/
//#define TIMES_TO_INQUIRE 10
#define MAX_TURN_BYTE			32*1024
#define MAX_CHANNEL_NUM			128
#define MAX_SENSOR_NUM 			4
#define POINT_NUM 				100
#define CYCLICAL_BUFFER_SIZE	10


enum dev_stat {
	dev_NONE,
	dev_START,
	dev_CONNECT,
	dev_UNCONNECT,
	dev_ERROR
};

enum error_stat {
    SOCKET_OK,
	SOCKET_DATA_WRONG,
	SOCKET_CONNECT_TIMEOUT,
	SOCKET_DATA_TIMEOUT,
	SOCKET_INIT_ERROR
};

enum protocol_dir_stat {
	PSTAT_NONE,
	PSTAT_SEND,
	PSTAT_RECV
	//PSTAT_BOTH
};

enum protocol_cmd_stat {
	PSTAT_NO_DEVICE			= 0xFF,
	PSTAT_DEVICE_OPEN,
	PSTAT_INIT_SOCKET,
	PSTAT_SHAKEHAND,		//= 0x01,
	PSTAT_DATA,
    PSTAT_SET_RATE
};

//!< 工频和基线漂移的处理，两者的选项相或
enum filter_selector {
	FILTER_NONE = 1,		//<! no 50hz filter
	FILTER_BUTTERWORTH ,
	FILTER_FOURIER,
	FILTER_RLS,
	FILTER_BASELINE_NONE =0x0100,		//<! don't deal with baseline bias
	FILTER_BASELINE_YES	=0x0200			//<! filter the baseline bias
};

struct sEMGdata{
    double point[POINT_NUM];
};

struct raw_sEMGdata{
    double point[POINT_NUM];
};

struct cyc_buffer {	/* internal cyclical buffer (one channel) */
	//unsigned char packet_class;
	unsigned int valid_amount;		                        /* valid amount of data */
	unsigned int header;			                        /* header pointer of buffer */
	unsigned char channel_id;			                    /* channel information */
	struct sEMGdata data[CYCLICAL_BUFFER_SIZE];				/* proccessed data array */
    struct raw_sEMGdata raw_data[CYCLICAL_BUFFER_SIZE];
};

// motion sensor
struct sensorData {
	int16_t x[5];
	int16_t y[5];
	int16_t z[5];
};

struct sensorCycBuffer {
	unsigned int valid_amount;
	unsigned int header;
	struct sensorData mag[CYCLICAL_BUFFER_SIZE]; 	
	struct sensorData gyro[CYCLICAL_BUFFER_SIZE];
	struct sensorData acc[CYCLICAL_BUFFER_SIZE];
};

struct protocol_stat {
	unsigned int cmd_stat;		/* the next comand state */
	unsigned int sub_stat;		/* sub type stat mainly used for cmd */
	//unsigned int inquire;		/* times to indicate the despired times */
	int error;
	unsigned char *pdata;		/* packet data */
};

struct thread_args { /* auxiliary thread arguments */
	bool threadrun;
	std::mutex cs_mutex;
	bool is_set;
	//unsigned int num[MAX_sEMG_CHANNEL];
	//struct sample_set set;
};

struct dev_info {	/* device information */
	unsigned int  version;
	unsigned char channel_num;   /* channel number in root */
	unsigned char AD_rate;  /*AD rate*/
	int dev_stat;					/* current device stat */
	char ip[20];						/* device ip addr*/
	int hdev;					/* device handle */
};




void start_comu_thread(unsigned int *tid, struct thread_args *args);
void stop_comu_thread(int ht, struct thread_args *args);
//UINT WINAPI comu_thread_proc(struct thread_args *args);
void comu_thread_proc(void *pargs);

void init_dll(void);
bool init_socket(struct dev_info *pdi,struct protocol_stat *pstat);

//bool shakehand_sec(struct dev_info *pdi, struct protocol_stat *pstat);
bool protocol_handler(struct dev_info *pdi, bool *prun);
bool shakehand(struct dev_info *pdi, struct protocol_stat *pstat);
bool update_data(struct dev_info *pdi, struct protocol_stat *pstat);
bool update_cbuffer(struct dev_info *pdi, struct protocol_stat *pstat);
void parse_data(unsigned char *pdata, struct cyc_buffer *pcb, int num);
void parse_sensor_data(unsigned char *pdata, struct sensorCycBuffer *pcb);
bool Is_connect_ready(void);
bool Is_send_ready(void);
bool Is_recv_ready(void);
bool error_handler(struct protocol_stat *pstat);
void uninit(void);

#endif
