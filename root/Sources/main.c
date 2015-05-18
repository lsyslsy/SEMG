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
#include <assert.h>
#include <sched.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "../Headers/main.h"
#include "../Headers/mytime.h"
#include "../Headers/socket.h"
#include "../Headers/collect.h"
#include "../Headers/led.h"
#include "../Headers/semg_debug.h"
#include "../Drivers/USB/usb_semg.h"
#include "../Headers/process.h"

extern pthread_mutex_t mutex_send;
extern pthread_cond_t cond_send;
extern pthread_mutex_t mutex_tick;
extern pthread_cond_t cond_tick;

struct root root_dev;
struct branch branches[BRANCH_NUM] = {{0}};//
unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE] =
{{0}}; // branch data pool



unsigned int active_branch_count = 0;
//init the root status
int root_init(void)
{
	root_dev.version = ROOT_VERSION;
	root_dev.channel_num = CHANNEL_NUM;
	root_dev.AD_rate = RATE_1K;

	pthread_mutex_init(&mutex_tick, NULL);
	pthread_cond_init(&cond_tick, NULL);

	pthread_mutex_init(&mutex_send, NULL);
	pthread_cond_init(&cond_send, NULL);

	return 0;
}//root_init()

/**
 * init the branches' status, 动态绑定branch到usb device
 */
void branch_init()
{
	int i, retval, err;
	int fd;
	int n;
	int current_fn = 22222;
	char 			strbuf[20] ={0};
	memset(branches, 0, sizeof(branches));

	for (i = 0; i < BRANCH_NUM; i++) {
		branches[i].num = i;
		branches[i].devfd = -1;
		branches[i].data_pool = data_pool[0];
		branches[i].has_shakehanded = FALSE;
		branches[i].is_connected = FALSE;
		branches[i].need_shakehand = TRUE;
		branches[i].timeout = 0;
		branches[i].expected_fn = 11111;
		branches[i].waitms = 0;
	}

	for (i = 0; i < 30; i++) {
		sprintf(strbuf, "/dev/semg-usb%d", i);
		fd = open(strbuf, O_RDWR);
		if (fd < 0) {
			close(fd);
			continue;
		}
		err = ioctl(fd, USB_SEMG_GET_BRANCH_NUM, NULL);
		if (err < 0) {
			DebugError("ioctl semg_usb%d error: %s\n", i, strerror(errno));
			close(fd);
			continue;
		}
		n = err;
		if (n <0 || n >= BRANCH_NUM) { // 我可不想被坑到
			DebugError("ioctl semg_usb%d error: invalid branch number: %d\n", i, n);
			close(fd);
			continue;
		}

		if (branches[n].is_connected) { //重复连接
			DebugError("detect error: branch %d duplicated\n", n);
			close(fd);
			continue;
		}

		branches[n].is_connected = TRUE;
		branches[n].devfd = fd;
		active_branch_count++;
		DebugInfo("binding semg-usb%d to branch %d\n", i, n);

	}

	if (active_branch_count > 0)
		DebugInfo("total %d branches valid\n", active_branch_count);
	else {
		DebugError("no branches valid\n");
		exit(EXIT_FAILURE);
	}

	// 找到8个里面current最大的那个,并加500ms,作为1次同步过程

	for (i = 0; i < BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;
		retval = ioctl(branches[i].devfd, USB_SEMG_GET_CURRENT_FRAME_NUMBER, NULL);
		if (retval < 0) {
			branches[i].is_connected = FALSE;
			DebugError("branches%d ioctl: get current_fn failed\n", i);
		} else {
			current_fn = retval;
			break;
		}
	}
	if (current_fn == 22222) {
		DebugError("no branches valid after ioctl get current_fn\n");
		exit(EXIT_FAILURE);
	}


	current_fn = (current_fn+500)%1024; 		// delay 500 ms
	// set all to sync
	for (i = 0; i < BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;

		retval = ioctl(branches[i].devfd, USB_SEMG_SET_EXPECTED_FRAME_NUMBER, current_fn);
		if (retval < 0) {
			branches[i].is_connected = FALSE;
			active_branch_count--;
			DebugError("branches%d ioctl: set expected_fn failed\n", i);
			continue;
		}

		retval = ioctl(branches[i].devfd, USB_SEMG_GET_EXPECTED_FRAME_NUMBER, NULL);
		if (retval < 0) {
			branches[i].is_connected = FALSE;
			active_branch_count--;
			DebugError("branches%d ioctl: get expected_fn failed\n", i);
			continue;
		} else if (retval != current_fn){
			branches[i].is_connected = FALSE;
			active_branch_count--;
			DebugError("branches%d ioctl: get expected_fn not equals setted, set: %d, get: %d\n", i, current_fn, retval);
			continue;
		}
			DebugInfo("branches%d ioctl: get expected_fn not equals setted, set: %d, get: %d\n", i, current_fn, retval);

	}

	if (active_branch_count > 0)
		DebugInfo("total %d branches valid\n", active_branch_count);
	else {
		DebugError("no branches valid after ioctls\n");
		exit(EXIT_FAILURE);
	}


	//TODOO   要动态的查找(通过和下位机的通信)对应number的device,并赋值给.device,


}//branch_init()

void show_thread_priority(pthread_attr_t *attr, int policy)
{
	int priority = sched_get_priority_max(policy);
	assert(priority != -1);
	printf("max_priority=%d\n", priority);
	priority = sched_get_priority_min(policy);
	assert(priority != -1);
	printf("min_priority=%d\n", priority);
	struct sched_param param;
	pthread_getschedparam(pthread_self(), &policy,
                         &param);
	printf("Current thread's priority:%d\n",param. __sched_priority);
}

/**
 *main() funciton: contain 9 pthreads(1 socket send thread &&
 *               8 branch collect threads), branch collect
 *               threads have higher priority. All the pthreads
 *      	  are triggered by the timer(100ms per).
 **/
int main()
{

	int ret;
	int i;
	struct sched_param param;
	pthread_t p_socket;
	pthread_t p_branch;
	pthread_t p_process;
	pthread_attr_t thread_attr;
	printf("The program is compiled on %s\n" , __DATE__);
	i = getuid();
	if (i == 0)
		printf("The current user is root\n");
	else {
		DebugError("The current user is not root\n");
		exit(EXIT_FAILURE);
	}
	printf("the main thread's pid is %lu\n",pthread_self());
	ret = root_init();//linux device init
	if (ret) {
		DebugError("root_init error!\n");
		exit(EXIT_FAILURE);
	}
	if (process_init()) {
		DebugError("process_init failed!\n");
		exit(EXIT_FAILURE);
	}
	branch_init(); //8 branch init

	//SCHED_FIFO适合于实时进程，它们对时间性要求比较强，而每次运行所需要的时间比较短。
	//一旦这种进程被调度开始运行后，就要一直运行直到自愿让出CPU或者被优先权更高的进程抢占其执行权为止，没有时间片概念。
	//SCHED_RR对应“时间片轮转法”，适合于每次运行需要较长时间的实时进程。
	//SCHED_OTHER适合于交互式的分时进程。这类非实时进程的优先权取决于两个因素：
	//进程剩余时间配额和进程的优先数nice（优先数越小，其优先级越高）。nice的取值范围是19~-20。
	// 这个RR调度是针对相同优先级的，所以高优先级不释放，低优先级的线程还是无法执行，在这里结果和FIFO一样。

	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	//必需设置inher的属性为 PTHREAD_EXPLICIT_SCHED，否则设置线程的优先级会被忽略
	pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);

	// branch thread
	param.sched_priority = 20;
	pthread_attr_setschedparam(&thread_attr, &param);
	if( pthread_create(&p_branch, &thread_attr, (void *) FunBranch, NULL)) {
		perror("Create collect thread branch error");
		exit(EXIT_FAILURE);
	}
	DebugInfo("Create collect thread branch, tid:%lu\n", p_branch);
	usleep(100000); // 100ms

	//process thread
	param.sched_priority = 10;
	pthread_attr_setschedparam(&thread_attr, &param);
	if( pthread_create(&p_process, &thread_attr, (void *) process, NULL)) {
		perror("Create process thread branch error");
		exit(EXIT_FAILURE);
	}
	DebugInfo("Create process thread branch, tid:%lu\n", p_process);
	usleep(100000); // 100ms

	// coummunication thread
	param.sched_priority = 15;
	pthread_attr_setschedparam(&thread_attr, &param);
	if( pthread_create(&p_socket, &thread_attr, (void *) FunSocket, NULL)) {
		perror("Create socket thread branch error");
		exit(EXIT_FAILURE);
	}
	DebugInfo("Create socket thread branch, tid:%lu\n", p_socket);

	pthread_attr_destroy(&thread_attr);

	sleep(2); // 方便看信息

	init_sigaction(); // 设置信号处理函数
	if (init_timer() < 0 ) // 定时器
		exit(EXIT_FAILURE);

	//uninit
	pthread_join(p_branch, NULL);
	pthread_join(p_process, NULL);
	pthread_join(p_socket, NULL);
	process_uninit();
	pthread_mutex_destroy(&mutex_tick);
	pthread_cond_destroy(&cond_tick);
	pthread_mutex_destroy(&mutex_send);
	pthread_cond_destroy(&cond_send);
    DebugInfo("the end");
	return 0;
}//main()