/*
 * time.c: The time file for root board of the sEMG project
 *
 *  All right reserved.
 *
 *  Created: HY@2013.5.10
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "../Headers/main.h"
#include <time.h>
#include "../Headers/mytime.h"
#include "../Headers/led.h"
#include "../Headers/semg_debug.h"
#include "../Drivers/USB/usb_semg.h"
#include <errno.h>

pthread_cond_t cond_tick; // 通知100ms中断
pthread_mutex_t mutex_tick; // protect capture_state;
int capture_state = 2; // // 0:start, 1:processing, 2:finish
extern struct branch branches[BRANCH_NUM];
timer_t timerid;
struct itimerspec its;

// ioctl 会和branch里面的read冲突?内核驱动里面已经加锁了啊
// expected_fn:782, current_fn:688,expire: 100ms
// expected_fn:882, current_fn:789,expire: 99ms
// expected_fn:982, current_fn:888,expire: 100ms
// expected_fn:58, current_fn:988,expire: 100ms
// expected_fn:158, current_fn:65,expire: 99ms
// expected_fn:258, current_fn:164,expire: 100ms

// TODO 有并发问题,但不大
int mysync()
{
    int retval = 0;
    int i;
    int expected_fn;
    int current_fn;
    unsigned long expire;

    // get expected fn
    for (i = 0; i < BRANCH_NUM; i++) {
        if (branches[i].is_connected == FALSE)
            continue;
         //要看驱动，好像返回类型有问题
        retval = ioctl(branches[i].devfd, USB_SEMG_GET_EXPECTED_FRAME_NUMBER, NULL);
        if (retval < 0) {
            branches[i].is_connected = FALSE;
            DebugError("branches%d ioctl: get expected_fn failed in %s, retval: %d\n", i, __func__, retval);
            exit(1);
        } else {
            expected_fn = retval;
            retval = ioctl(branches[i].devfd, USB_SEMG_GET_CURRENT_FRAME_NUMBER, NULL);
            if (retval < 0) {
                branches[i].is_connected = FALSE;
                DebugError("branches%d ioctl: get current_fn failed in %s, retval: %d\n", i, __func__, retval);
                exit(1);
            } else {
                current_fn = retval;
                break;
            }
        }
    }
    if (i == BRANCH_NUM) { // TODO
        DebugError("get no expected_fn failed in%s\n", __func__);
        exit(1);
    }
    // set timer delay = expected fn - current + 5
    expire = (expected_fn + 1030 - current_fn) % 1024;
    expire = expire * 1000000;
    its.it_value.tv_sec = expire / 1000000000;
    its.it_value.tv_nsec = expire % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if ((expire/1000000) < 60) {  // 处理来不及
        DebugError("not enough time to process, skip this turn\n");
        retval = -2;
    }
    DebugWarn("expected_fn:%4d, current_fn:%4d,expire: %3ldms\n", expected_fn, current_fn, expire/1000000);
    if (timer_settime (timerid, 0, &its, NULL) == -1) {
        perror("timer_settime2 error");
        retval = -1;
        return retval;
    }
    return retval;
}

void timeout_info(int signo)
{
    if (mysync() < 0 ) // error or not enough time
        return;

    pthread_mutex_lock(&mutex_tick);
    if (capture_state == 1) {
        DebugError("danger:still proccessing\n");
    }
    if (capture_state == 0) {
        DebugError("fatal:why no processing\n");
    }
    capture_state = 0; // set state to start
    pthread_cond_signal(&cond_tick); // notify the condition to collector
    pthread_mutex_unlock(&mutex_tick);

  // printf("the timer lost %d signals\n",k);
  //be sure to run in main thread
 //printf("the timer run in pid is %d\n",pthread_self());
//usleep(1);//加了也没用

    //printf("100ms time is up!\n");
}

/* init sigaction */
void init_sigaction(void)
{
     struct sigaction act;
     act.sa_handler = timeout_info;//指定信号处理海曙
     act.sa_flags   = 0;
     act.sa_flags |= SA_RESTART; // 让被中断的系统调用自动恢复
     sigemptyset(&act.sa_mask);
     sigaction(SIGUSR1, &act, NULL);
}

/* init */
int init_timer(void)
{
    int retval = -1;
    struct sigevent sev;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1; //SIGALRM; // 不必也不应该用可靠信号[SIGRTMIN, SIGRTMAX],只需要一次,中间丢了不管
    sev.sigev_value.sival_ptr = &timerid;
    //sev.sigev_notify_function = timeout_info;
    sev.sigev_notify_attributes = NULL;

    /* create timer */
    if ((retval = timer_create (CLOCK_REALTIME, &sev, &timerid)) < 0)
    {
        perror("timer_create, error");
        return retval;
    }

   // if (*timerid == -1)
      //  printf("timer_create error, id is -1\n");

    struct itimerspec its;
     /* Start the timer */
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if ((retval = timer_settime (timerid, 0, &its, NULL) == -1)) {
        perror("timer_settime error");
        return retval;
    }
    return 0;
}