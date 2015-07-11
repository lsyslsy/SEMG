#ifdef UNICODE
#undef UNICODE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sEMG_DLL.h"
#include "socket.h"

#include "semg_debug.h"
#include <thread>
#include <chrono>

using namespace std::chrono;

const char dll_info[] = { "Program version V1.0.0\nAther: Hu Ye, 2013.05.20" };

//HANDLE hthread = INVALID_HANDLE_VALUE;			/* the handle of the auxiliary thread */
struct thread_args targs;						/* thread arguments */

extern struct dev_info device;
extern unsigned int timestamp;
extern unsigned int lose_num;
extern unsigned char spi_stat[2];
extern bool inited;
extern std::mutex data_mutex;			/* for pcbuffer use */
extern struct cyc_buffer *pcbuffer[MAX_CHANNEL_NUM];

extern int Filter_Options;
extern void(*notify_data)(void);
extern void do_nothing();

// std::thread *com_thread;

sEMGAPI bool  sEMG_open(bool wait, const char *ip, int filter_options)
{

	unsigned int threadid;
	bool ret = true;

	Filter_Options = filter_options;
	strcpy(device.ip, ip);
	//int filter_options
	std::thread t1{ start_comu_thread, &threadid, &targs };
	t1.detach();

	//if (hthread == INVALID_HANDLE_VALUE || targs.threadrun != true)// || targs.is_set != true)
	//{
	//	DebugError(TEXT("hthread error!!\n"));
	//	return false;
	//}
	while (wait) {
		std::this_thread::sleep_for(seconds{ 1 });//1000ms

		if (device.dev_stat == dev_CONNECT) {
			DebugInfo("success!!\n");
			wait = false;
		} else if (device.dev_stat == dev_UNCONNECT ||
			device.dev_stat == dev_ERROR) {
			DebugError("final failed!!\n");
			wait = false;
			ret = false;
		} else {
			DebugError("final failed!!\n");
			wait = false;
			ret = false;
		}
	}
	return ret;
}

/**
 * close the device, stop the auxiliary thread
 *
 * @note 当前实现不完全
 * @return true if operation successful, else returns false
 */
sEMGAPI bool  sEMG_close(void)
{
	//	if (hthread != INVALID_HANDLE_VALUE)
	//	{
	//		stop_comu_thread(hthread, &targs);
	//		hthread = INVALID_HANDLE_VALUE;
	//	}
	stop_comu_thread(0, &targs);
	return true;
}

sEMGAPI bool  sEMG_reset(bool wait, char *ip, int filter_options)
{
	if (!sEMG_close())
		return false;
	//init_dll();
	return (sEMG_open(wait, ip, filter_options));
}


sEMGAPI void set_data_notify(void(*pfunc)(void))
{
	notify_data = pfunc;
	return;
}


sEMGAPI void reset_data_notify(void)
{
	notify_data = do_nothing;
	return;
}

sEMGAPI void  get_dll_info(char *pinfo)
{
	int i;
	for (i = 0; i < sizeof(dll_info); i++) {
		*pinfo = dll_info[i];
		pinfo++;
	}
	return;
}

sEMGAPI void  clearbuffer(void)
{
	//	int i;
	//	if (targs.threadrun == false)
	//		return;
	//
	//	data_mutex.lock();
	//	for (i=0; i<MAX_CHANNEL_NUM; i++)
	//	{
	//		if(pcbuffer[i])
	//			free(pcbuffer[i]);
	//		pcbuffer[i] = NULL;
	//	}
	//	data_mutex.unlock();
	return;
}

sEMGAPI int   get_dev_stat(void)
{
	return device.dev_stat;
}


sEMGAPI void   get_spi_stat(void *p)
{
	unsigned char *pp = (unsigned char *)p;
	pp[0] = spi_stat[0];
	pp[1] = spi_stat[1];
}

sEMGAPI unsigned char get_channel_num(void)
{
	return device.channel_num;
}

sEMGAPI unsigned char get_AD_rate(void)
{
	return device.AD_rate;
}

sEMGAPI unsigned int  get_timestamp(void)
{
	return timestamp;
}

sEMGAPI unsigned int  get_losenum(void)
{
	return lose_num;
}


sEMGAPI int get_sEMG_data(int channel_id, unsigned int size, void *pd)
{
	int i;
	int num;//the actual count of data to read
	unsigned int header;
	struct sEMGdata *psn;
	num = 0;
	//TODO 优化掉数据同步问题
	psn = (struct sEMGdata *)pd;
	if (channel_id >= MAX_CHANNEL_NUM)
		return num;
	data_mutex.lock();
	if (pcbuffer[channel_id] == NULL)
		return num;
	if (pcbuffer[channel_id]->valid_amount) {  //any valid channel data
		if (size < pcbuffer[channel_id]->valid_amount)
			num = size;
		else
			num = pcbuffer[channel_id]->valid_amount;

		header = (pcbuffer[channel_id]->header + CYCLICAL_BUFFER_SIZE - pcbuffer[channel_id]->valid_amount + 1) % CYCLICAL_BUFFER_SIZE;//计算数据指针初始位置
		pcbuffer[channel_id]->valid_amount -= num;

		for (i = 0; i < num; i++) {
			((struct sEMGdata*)pd)[i] = pcbuffer[channel_id]->data[header];
			header = (++header) % CYCLICAL_BUFFER_SIZE;
		}
	}

#ifdef DLL_DEBUG_MODE
	//OutputDebugPrintf("DEBUG_INFO | channelID = %d num = %d, valid_amount = %d\n",channel_id,num,pcbuffer[channel_id]->valid_amount);
#endif

	data_mutex.unlock();
	return num;
}