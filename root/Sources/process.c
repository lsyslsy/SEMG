/*
 * process.c: The main file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: 2015-5-13 yao
 */
#include <stdio.h>
#include <errno.h>

#include "../Headers/main.h"
#include "../Headers/collect.h"
#include "../Headers/led.h"
#include "../Headers/semg_debug.h"

extern unsigned char spi_recv_buf[BRANCH_NUM][BRANCH_BUF_SIZE];

static int ParseDataPacket(unsigned char *p, int n);
static void print_data(unsigned char * pbuf, int branch_num);
struct work_queue semg_queue;
/**
 * Init processor
 */
int process_init()
{
	int ret = 0;
	if (ret = queue_init(&semg_queue, 2)) {
		return ret;
	}
	return ret;
}

 void  process(void *parameter)
 {
 	int branch_num;
 	while (1) {
 		// SEMG data process
	 	for (branch_num = 0; branch_num < BRANCH_NUM; branch_num++) {
	 		pbuf = spi_recv_buf[branch_num];

	 		// data parse
	 		int tmp = ParseDataPacket(pbuf, branch_num);
			if (tmp == 0) {
				print_data(pbuf, branch_num);
				//Data verified, then copy to socket
				memcpy(bx->data_pool, pbuf, BRANCH_BUF_SIZE);
			}
			else {
				/***********************tmp*****************/
				bx->data_pool[0] = 0xee; //data error
				DebugWarn("Data Packet from Branch%d have wrong bytes:%d\n",
						branch_num, tmp);
				printf("read:%d,%x,%x,%x,%x,%x\n", size, pbuf[0], pbuf[1], pbuf[2],
						pbuf[3], pbuf[3256]);
			}

			// signal process

			// data compress

			// data pack
			if(data_pool[i][0] == 0x48)
				pbuf[0] = 0x48;
			if(data_pool[i][0] == 0xee)
				pbuf[1] = pbuf[1] | 0x01 << i;
			//temp_size = (data_pool[i][2] << 8) + data_pool[i][3];//每个Branch的长度
			//if (temp_size == BRANCH_DATA_SIZE)//不应该在这里验证
			//{
				///##数据包出错改怎么处理##////
				memcpy(p, &data_pool[i][BRANCH_Header_SIZE], BRANCH_DATA_SIZE);

			//}
			*psize += BRANCH_DATA_SIZE;//搞毛
			p += BRANCH_DATA_SIZE;//不管成不成功都要加上去
			*p = DATA_END;
			*psize += 1;

			// start to send data
			// NOTE: 可能有竞争，打包和发送
			// 发送消息给socket线程，
			pthread_mutex_lock(&mutex_send);
			send_ready = 1;
			pthread_cond_signal(&cond_send);
			pthread_mutex_unlock(&mutex_send);

	 	}

	 	// motion sensor process
	 }

 }

 /**
 * Parse the data packet from branch.
 * 验证数据包格式是否正确.
 * @param p 传入数据包地址指针.
 * @param n 通道编号.
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
	p += 9;
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
		errno = EINVAL
		return -EINVAL;
	}
	// 循环缓冲区总有1个区域是没用的，最多容纳size-1个
	wq->jobs = malloc((size_t)(cap + 1) * sizeof(struct job))
	if (!wq->jobs) {
		return errno = -ENOMEM;
	}
	wq->size = cap + 1;
	wq->writer = 0;
	wq->reader = 0;
	pthread_cond_init(&wq->wait_data, NULL);
	pthread_cond_init(&wq->wait_room, NULL);

	return 0;
}

/**
 * Take a job to finish
 */
struct job *queue_get(struct  work_queue *const wq)
{
	struct job *j;

	pthread_mutex_lock(&q->lock);
	while (q->reader == q->writer) // while empty
		pthread_cond_wait(&q->wait_data);
	j = &wq->jobs[q->reader];
	q->reader = (q->reader + 1) % size;
	pthread_cond_signal(&q->wait_room);
	pthread_mutex_unlock(&q->lock)

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
		pthread_cond_wait(&wq->wait_room);
	j = &wq->jobs[wq->writer];
	j->type = type;
	j->branch_num = branch_num;
	wq->writer = (wq->writer + 1) % wq->size;
	pthread_cond_signal(&wq->wait_data);
	pthread_mutex_unlock(&qw->lock);

	return 0;
}