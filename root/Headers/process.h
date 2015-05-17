/*
 * process.h: Implement data parse, signal processing, data compression
 *
 *	All right reserved.
 *
 *	Created: 2015.5.13 yao
 */
#ifndef _PROCESS_H
#define _PROCESS_H

struct job {
	int type; // semg 1, motion 2
	int branch_num;
};

// work queue
struct work_queue
{
	int size; // size of jobs
	int writer;
	int reader;
	struct job *jobs; // 容量为size
	pthread_mutex_t lock;
	pthread_cond_t wait_room;
	pthread_cond_t wait_data;
};

int queue_put(struct work_queue *const wq, int type, int branch_num);
struct job *queue_get(struct  work_queue *const wq);
int queue_init(struct work_queue *const wq, int cap);
int queue_uninit(struct work_queue *const wq);
int process_init();
int process_uninit();
void  process(void *parameter);

#endif