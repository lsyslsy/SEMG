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
#include "../Headers/pwm_sync.h"
#include "../Drivers/USB/usb_semg.h"
 
struct root root_dev;
struct branch branches[BRANCH_NUM] = {{0}};//
unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE] =
{{0}}; // branch data pool
int branch_priority[BRANCH_NUM] =
{ 19, 18, 17, 16, 15, 14, 13, 12 };//优先数越大优先级越大
// char used_branch[BRANCH_USED_NUM] = {-1, -1, -1, -1, -1, -1, -1, -1};
// static char thead_map_branch[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
// each elment is a branch num, should use lock sync
pthread_mutex_t mutex_buff;
pthread_cond_t cond_tick;

#ifdef ARM_VERSION
int Branch_Read_fd = -1;
int pwm_sync_fd = -1;
#endif

//init the root status
int root_init(void)
{
	root_dev.version = ROOT_VERSION;
	root_dev.channel_num = CHANNEL_NUM;
	root_dev.AD_rate = RATE_1K;
#ifdef ARM_VERSION
	Branch_Read_fd = open("/dev/sEMG_Input", O_RDWR); // 打开标志输入脚0～7
	if (Branch_Read_fd < 0)
	{
		perror("Can't open /dev/sEMG_Input");
		return -1;
	}
#endif
//#endif
	pthread_mutex_init(&mutex_buff, NULL);
	pthread_cond_init(&cond_tick, NULL);
	return 0;
}//root_init()

#ifdef ARM_VERSION
int pwm_sync_init(void)
{
	int err;
	pwm_sync_fd = open("/dev/pwm_sync", O_RDONLY);
		if(pwm_sync_fd <0) {
		perror("open pwm_sync");
		return -1;
	}

	err = fcntl(pwm_sync_fd, F_SETOWN, getpid());
	if(err < 0) {
		perror("set owner");
		return -1;
	}
	int oflags = fcntl(pwm_sync_fd, F_GETFL);
	err = fcntl(pwm_sync_fd, F_SETFL, oflags | FASYNC);
	if(err < 0) {
		perror("set async");
		return -1;
	}
	err += ioctl(pwm_sync_fd, PWM_IOC_STOP,NULL);
	err += ioctl(pwm_sync_fd, PWM_IOC_SET_FREQ, 10);
	err += ioctl(pwm_sync_fd, PWM_IOC_START,NULL);
	if(err) {
		perror("ioctl pwm_sync");
		return -1;
	}
	return 0;

}
#endif

/**
 * init the branches' status, 动态绑定branch到usb device
 */
void branch_init()
{
	int i, j, retval, err;
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
		DebugInfo("binding semg-usb%d to branch %d\n", i, n);

	}

	j = 0;
	// 再看下哪些设备是否都注册的
	for (i = 0; i < BRANCH_NUM; i++) {
		if (branches[i].is_connected == TRUE)
			j++;
	}
	if (j > 0)
		DebugInfo("total %d branches valid\n", j);
	else {
		DebugError("no branches valid\n");
		exit(EXIT_FAILURE);
	}

	// 找到8个里面expected最大的那个,并加200ms,作为1次同步过程
	
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


	current_fn = (current_fn+200)%1024; 		// delay 200 ms
	// set all to sync
	for (i = 0; i < BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;

		retval = ioctl(branches[i].devfd, USB_SEMG_SET_EXPECTED_FRAME_NUMBER, current_fn);
		if (retval < 0) {
			branches[i].is_connected = FALSE;
			DebugError("branches%d ioctl: set expected_fn failed\n", i);
			continue;
		} 

		retval = ioctl(branches[i].devfd, USB_SEMG_GET_EXPECTED_FRAME_NUMBER, NULL);
		if (retval < 0) {
			branches[i].is_connected = FALSE;
			DebugError("branches%d ioctl: get expected_fn failed\n", i);
			continue;
		} else if (retval != current_fn){
			branches[i].is_connected = FALSE;
			DebugError("branches%d ioctl: get expected_fn not equals setted\n", i);
			continue;
		}
	}
	j = 0;
	// 看下哪些设备还没注册的
	for (i = 0; i < BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			DebugError("error: can't find branch %d\n", i);
		else
			j++;
	}
	if (j > 0)
		DebugInfo("total %d branches valid\n", j);
	else {
		DebugError("no branches valid after ioctls\n");
		exit(EXIT_FAILURE);
	}

	
	//TODOO   要动态的查找(通过和下位机的通信)对应number的device,并赋值给.device,


}//branch_init()
static void show_thread_priority(pthread_attr_t *attr, int policy)
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
	printf("the main thread's pid is %ld\n",pthread_self());
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
	printf("The program is compiled on %s\n" , __DATE__);
	i = getuid();
	if (i == 0)
		printf("The current user is root\n");
	else
	{
		DebugError("The current user is not root\n");
		exit(EXIT_FAILURE);
	}
	ret = root_init();//linux device init
	if (ret)
	{
		DebugError("root_init error!\n");
		exit(EXIT_FAILURE);
	}
	branch_init(); //8 branch init
	sleep(2); //方便看信息
	struct sched_param param;
	pthread_t p_socket;
	pthread_attr_t attr_socket;

	pthread_t p_branch[BRANCH_NUM];
	pthread_attr_t attr_branch[BRANCH_NUM];

	pthread_attr_init(&attr_socket);
	for (i = 0; i < BRANCH_NUM; i++)
		pthread_attr_init(&attr_branch[i]);

	param.sched_priority = 1;
	//SCHED_FIFO适合于实时进程，它们对时间性要求比较强，而每次运行所需要的时间比较短。
	//一旦这种进程被调度开始运行后，就要一直运行直到自愿让出CPU或者被优先权更高的进程抢占其执行权为止，没有时间片概念。
	//SCHED_RR对应“时间片轮转法”，适合于每次运行需要较长时间的实时进程。
	//SCHED_OTHER适合于交互式的分时进程。这类非实时进程的优先权取决于两个因素：
	//进程剩余时间配额和进程的优先数nice（优先数越小，其优先级越高）。nice的取值范围是19~-20。
	pthread_attr_setschedpolicy(&attr_socket, SCHED_RR);
	pthread_attr_setschedparam(&attr_socket, &param);
	//必需设置inher的属性为 PTHREAD_EXPLICIT_SCHED，否则设置线程的优先级会被忽略
	pthread_attr_setinheritsched(&attr_socket, PTHREAD_EXPLICIT_SCHED);//do not inherit father's attr
   // i=0;
	 show_thread_priority(&attr_socket,  SCHED_RR);
	for (i = 0; i < BRANCH_NUM; i++)
	{
		if (branches[i].is_connected == FALSE)
			continue;
		pthread_attr_init(&attr_branch[i]);
		param.sched_priority = branch_priority[i];
		pthread_attr_setschedpolicy(&attr_branch[i], SCHED_RR);
		pthread_attr_setschedparam(&attr_branch[i], &param);
		pthread_attr_setinheritsched(&attr_branch[i], PTHREAD_EXPLICIT_SCHED);
		//pthread_create(&p_socket,&attr_socket,(void *)FunBranch,(void *)i);
		if( pthread_create(&p_branch[i], &attr_branch[i], (void *) FunBranch,
				(void *) i))
			DebugError("Create thread branch%d error\n",i);
		DebugInfo("Create thread branch%d\n",i);
	}

	pthread_create(&p_socket, &attr_socket, (void *) FunSocket, NULL);
	/*pthread_t tid_test;
	pthread_attr_t attr_test;
	pthread_attr_init(&attr_test);
	struct sched_param param;
	param.sched_priority = 10;
	
	pthread_attr_setschedpolicy(&attr_test, SCHED_RR);   
	pthread_attr_setschedparam(&attr_test, &param);
	pthread_attr_setinheritsched(&attr_test, PTHREAD_EXPLICIT_SCHED);
    if(pthread_create(&tid_test, &attr_test, (void *)  thread_test, NULL) != 0)
	printf("create test thread failed\n");
	//
	//
	printf("app thread is running!\n");
	printf("the main thread pid is %d\n",pthread_self());
while(1)
 {
 usleep(100000);
 printf("the main thread pid is %d\n",pthread_self());;
 
 }*/

	init_sigaction();//设置信号处理函数
#ifdef ARM_VERSION
	if(pwm_sync_init()) return -1;
#else
	if (init_timer() < 0 )//定时器
		exit(EXIT_FAILURE);
#endif
	//uninit
   for (i = 0; i <BRANCH_NUM; i++) {
		if (branches[i].is_connected == FALSE)
			continue;
		pthread_join(p_branch[i], NULL);
		pthread_attr_destroy(&attr_branch[i]);
	}
	pthread_join(p_socket, NULL);
	pthread_attr_destroy(&attr_socket);
	pthread_mutex_destroy(&mutex_buff);
#ifdef ARM_VERSION
	//close(GPIO_L8_fd);
	close(pwm_sync_fd);
	close(Branch_Read_fd);
#endif
    DebugInfo("the end");
	return 0;
}//main()


