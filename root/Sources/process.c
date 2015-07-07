/*
 * process.c: The main file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: 2015-5-13 yao
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "../Headers/main.h"
#include "../Headers/collect.h"
#include "../Headers/led.h"
#include "../Headers/semg_debug.h"
#include "../Headers/socket.h"
#include "../Headers/process.h"

extern unsigned char semg_recv_buf[SEMG_NUM][SEMG_FRAME_SIZE];
extern unsigned char sensor_recv_buf[SENSOR_NUM][SENSOR_FRAME_SIZE];
extern unsigned char semg_pool[SEMG_NUM][SEMG_FRAME_SIZE];
extern unsigned char sensor_pool[SENSOR_NUM][SENSOR_FRAME_SIZE];
extern unsigned char sendbuff[MAX_TURN_BYTE];
extern unsigned int send_ready; // 0: not ready, 1: ready
extern pthread_mutex_t mutex_send;
extern pthread_cond_t cond_send;

static int ParseSemgDataPacket(unsigned char *p, int n);
static int ParseSensorDataPacket(unsigned char *p, int n);
static void print_data(unsigned char * pbuf, int branch_num);
struct work_queue semg_queue;
/**
 * Init processor
 */
int process_init()
{
	int ret = 0;
	if ((ret = queue_init(&semg_queue, 2))) {
		return ret;
	}
	return ret;
}

int process_uninit()
{
	queue_uninit(&semg_queue);
	return 0;
}

void  process(void *parameter)
{
	int i;
	int branch_num;
	struct job *job;
	struct branch *bx;
	unsigned char *pbuf;
	while (1) {

	// SEMG data process
	for (i= 0; i< SEMG_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;
	 	job = queue_get(&semg_queue);
	 	branch_num = job->branch_num;
 		if (job->type != 1 || branch_num != i) {
 			DebugError("fatal error occur, semg branch turn not as expected\n");
 			exit(1);
 		}
 		pbuf = semg_recv_buf[branch_num];
		bx = &branches[branch_num];

 		// data parse
 		int tmp = ParseSemgDataPacket(pbuf, branch_num);
		if (tmp == 0) {
			print_data(pbuf, branch_num);
			//Data verified, then copy to socket
			memcpy(bx->data_pool, pbuf, SEMG_FRAME_SIZE);
		} else {
			/***********************tmp*****************/
			bx->data_pool[0] = 0xee; //data error
			DebugWarn("Data Packet from Branch%d have wrong bytes:%d\n",
					branch_num, tmp);
			DebugWarn("read:%d,%x,%x,%x,%x,%x\n", bx->size, pbuf[0], pbuf[1], pbuf[2],
					pbuf[3], pbuf[3256]);
		}

		// signal process

		// data compress

		// data pack
		// NOTE: 边打包边发送会有竞争问题,不过经过分析后，情况出现概率小，只有当网速特别慢时
		if(semg_pool[branch_num][0] == 0x48)
			sendbuff[0] = 0x48;
		if(semg_pool[branch_num][0] == 0xee)
			sendbuff[1] = pbuf[1] | 0x01 << branch_num;

		///##数据包出错改怎么处理##////
		memcpy(sendbuff + 7 +branch_num * SEMG_DATA_SIZE,
			&semg_pool[branch_num][SEMG_HEADER_SIZE], SEMG_DATA_SIZE);
 	}

 	// motion sensor data
	for (i = SEMG_NUM; i< BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;
	 	job = queue_get(&semg_queue);
	 	branch_num = job->branch_num;
 		if (job->type != 2 || branch_num != i) {
 			DebugError("fatal error occur, sensor branch turn not as expected\n");
 			exit(1);
 		}
 		pbuf = sensor_recv_buf[branch_num - SEMG_NUM];
		bx = &branches[branch_num];

 		// data parse
 		int tmp = ParseSensorDataPacket(pbuf, branch_num);
		if (tmp == 0) {
			print_data(pbuf, branch_num);
		} else {
			DebugWarn("Data Packet from Branch%d have wrong bytes:%d\n",
					branch_num, tmp);
			DebugWarn("read:%d,%x,%x,%x,%x,%x\n", bx->size, pbuf[0], pbuf[1], pbuf[2],
					pbuf[3], pbuf[4]);
		}

		// data pack
		// if(data_pool[branch_num][0] == 0x48)
		// 	sendbuff[0] = 0x48;
		// if(data_pool[branch_num][0] == 0xee)
		// 	sendbuff[1] = pbuf[1] | 0x01 << branch_num;

		///##数据包出错改怎么处理##////
		memcpy(sendbuff + 7 + SEMG_NUM * SEMG_DATA_SIZE + (branch_num - SEMG_NUM) * SENSOR_DATA_SIZE,
			&sensor_pool[branch_num - SEMG_NUM][SENSOR_HEADER_SIZE], SENSOR_DATA_SIZE);
 	}
 	//else if (job->type == 2) {

 	// } else {
 	// 	DebugError();
 	// 	exit(1);
 	// }
 	// start to send data
 	// NOTE: 可能有竞争，打包和发送
 	// 发送消息给socket线程，
 	pthread_mutex_lock(&mutex_send);
	send_ready = 1;
	pthread_cond_signal(&cond_send);
 	pthread_mutex_unlock(&mutex_send);

 	// motion sensor process
 }

}

 /**
 * Parse the data packet of branch.
 * 验证数据包格式是否正确.
 * @param p 传入数据包地址指针.
 * @param n 通道编号.
 * @return 帧格式中错误的字节数
 */
static int ParseSemgDataPacket(unsigned char *p, int n)
{
	int i, j;
	int count = 0;
	if (p[0] != 0xb7)
		count++;
	if (p[1] != n)
		count++;
	if (p[2] != (SEMG_DATA_SIZE >> 8))
		count++;
	if (p[3] != (unsigned char) SEMG_DATA_SIZE)
		count++;
	p += 9;
	for (i = 0; i < CHANNEL_NUM_OF_SEMG; i++) {
		if (*p != 0x11)
			count++;
		p++;
		if (*p != i + n * CHANNEL_NUM_OF_SEMG)
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

 /**
 * Parse the data packet of sensor.
 * 验证数据包格式是否正确.
 * @param p 传入数据包地址指针.
 * @param n 通道编号.
 * @return 帧格式中错误的字节数
 */
static int ParseSensorDataPacket(unsigned char *p, int n)
{
	int count = 0;
	if (p[0] != 0xb8)
		count++;
	if (p[1] != n)
		count++;
	if (p[2] != (SENSOR_DATA_SIZE >> 8))
		count++;
	if (p[3] != (unsigned char) SENSOR_DATA_SIZE)
		count++;
	p += 9;
	p += SENSOR_DATA_SIZE;
	if (*p != 0xED)
		count++;
	return count;
}

static void print_data(unsigned char * pbuf, int branch_num)
{
	unsigned char buf_lbl = pbuf[7] & 0x01U;
	unsigned char buf_status = pbuf[7] >> 1 & 0x07U;
	unsigned char buf_ov = pbuf[8];

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
			branch_num, ch_buf_lbl, ch_status, buf_ov);
	DebugInfo("\033[40;34mbn\033[0m: %d, ", pbuf[1]);
	DebugInfo("\033[40;34mfn\033[0m: %d, ", pbuf[4] << 8 | pbuf[5]);
	DebugInfo("\033[40;34mwait\033[0m: %d\n", pbuf[6]);
	DebugInfo("\n");
}

int queue_init(struct work_queue *const wq, int cap)
{
	if (!wq || cap < 1) {
		errno = EINVAL;
		return -EINVAL;
	}
	// 循环缓冲区总有1个区域是没用的，最多容纳size-1个
	wq->jobs = malloc((size_t)(cap + 1) * sizeof(struct job));
	if (!wq->jobs) {
		return errno = -ENOMEM;
	}
	wq->size = cap + 1;
	wq->writer = 0;
	wq->reader = 0;
	pthread_mutex_init(&wq->lock, NULL);
	pthread_cond_init(&wq->wait_data, NULL);
	pthread_cond_init(&wq->wait_room, NULL);

	return 0;
}

int queue_uninit(struct work_queue *const wq)
{
	free(wq->jobs);
	pthread_mutex_destroy(&wq->lock);
	pthread_cond_destroy(&wq->wait_data);
	pthread_cond_destroy(&wq->wait_room);

	return 0;
}

/**
 * Take a job to finish
 */
struct job *queue_get(struct  work_queue *const wq)
{
	struct job *j;

	pthread_mutex_lock(&wq->lock);
	while (wq->reader == wq->writer) // while empty
		pthread_cond_wait(&wq->wait_data, &wq->lock);
	j = &wq->jobs[wq->reader];
	wq->reader = (wq->reader + 1) % wq->size;
	pthread_cond_signal(&wq->wait_room);
	pthread_mutex_unlock(&wq->lock);

	return j;
}

/**
 * Put a job to queue.
 * @param q 工作队列
 * @param type 任务类型
 * @param branch_num branch number
 * @return the status
 */
int queue_put(struct work_queue *const wq, int type, int branch_num)
{
	struct job *j;

	pthread_mutex_lock(&wq->lock);
	while (((wq->writer + 1) % wq->size) == wq->reader) // while full
		pthread_cond_wait(&wq->wait_room, &wq->lock);
	j = &wq->jobs[wq->writer];
	j->type = type;
	j->branch_num = branch_num;
	wq->writer = (wq->writer + 1) % wq->size;
	pthread_cond_signal(&wq->wait_data);
	pthread_mutex_unlock(&wq->lock);

	return 0;
}