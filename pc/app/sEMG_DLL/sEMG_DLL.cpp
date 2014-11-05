#ifdef UNICODE
	#undef UNICODE
#endif
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include "sEMG_DLL.h"
#include "socket.h"

#include "semg_debug.h"

const char dll_info[] = {"Program version V1.0.0\nAther: Hu Ye, 2013.05.20"};

HANDLE hthread = INVALID_HANDLE_VALUE;			/* the handle of the auxiliary thread */
struct thread_args targs;						/* thread arguments */

extern struct dev_info device;
extern unsigned int timestamp;
extern unsigned int lose_num;
extern unsigned char spi_stat[2]; 
extern BOOL inited;
extern CRITICAL_SECTION data_cs;
extern struct cyc_buffer *pcbuffer[MAX_CHANNEL_NUM];

extern int Filter_Options;
extern void (*notify_data)(void);
extern void do_nothing();
/**
 * open sEMG device, start the auxiliary thread
 *
 * @param[in] wait: if TRUE wait untill open the device,else return immediately
 * @param[in] ip The device ip address
 * @return TRUE if operation is successful,else return FALSE
 */
sEMGAPI BOOL  sEMG_open(BOOL wait, char *ip, int filter_options)
{
	
    unsigned int threadid;
	BOOL ret = TRUE;
	init_dll();
	Filter_Options = filter_options;
	strcpy(device.ip, ip);
	//int filter_options
	hthread = start_comu_thread(&threadid, &targs);
	//Debug("dfdf%d",10);
	if (hthread == INVALID_HANDLE_VALUE || targs.threadrun != TRUE)// || targs.is_set != true)
	{
		DebugError(TEXT("hthread error!!\n"));
		return FALSE;
	}
	while (wait) 
	{
		Sleep(1000);//1000ms

		if (device.dev_stat == dev_CONNECT) 
		{
			DebugInfo(TEXT("success!!\n"));
			wait = FALSE;
		} 
		else if (device.dev_stat== dev_UNCONNECT ||
			device.dev_stat == dev_ERROR) 
		{
			DebugError(TEXT("final failed!!\n"));
			wait = FALSE;
			ret = FALSE;
		}
		else
		{
			DebugError(TEXT("final failed!!\n"));
			wait = FALSE;
			ret = FALSE;
		}
	}
	return ret;
}

/**
 * close the device, stop the auxiliary thread
 *
 * @return TRUE if operation successful, else returns FALSE
 */
sEMGAPI BOOL  sEMG_close(void)
{
	if (hthread != INVALID_HANDLE_VALUE) 
	{
		stop_comu_thread(hthread, &targs);
		hthread = INVALID_HANDLE_VALUE;
	}
	if (inited)
		uninit();
	return TRUE;
}
/**
 * reset the device, restart the auxiliary thread
 *
 * @param[in] wait the parameter used to call wmsdas_open
 * @return  TRUE if operation successful, else returns FALSE
 */
sEMGAPI BOOL  sEMG_reset(BOOL wait, char *ip, int filter_options)
{
	if (!sEMG_close())
		return FALSE;
	//init_dll();
	return (sEMG_open(wait, ip, filter_options));
}
/**
 * set the callback function of notification
 *
 * @param[in] pfunc notification callback function pointer
 */
sEMGAPI void set_data_notify(void (*pfunc)(void))
{
   notify_data = pfunc;
   return;
}

/**
 * clear the callback function of notification and set donothing
 *
 */
sEMGAPI void reset_data_notify(void)
{
   notify_data = do_nothing;
   return;
}
/**
 * 
 *
 * @param[in] pinfo pointer to a char buffer that has a size larger than dll_info[]
 * @return the descroptor of this dll
 */
sEMGAPI void  get_dll_info(char *pinfo)
{
	int i;
	for (i=0; i<sizeof(dll_info); i++){
    	*pinfo = dll_info[i];
		pinfo++;
	}
	return;
}
/**
 * clear internal data buffer
 * @note this will lose all the data in the buffer
 */
sEMGAPI void  clearbuffer(void)
{
	int i;
	if (targs.threadrun == FALSE)
		return;

	EnterCriticalSection(&data_cs);
	for (i=0; i<MAX_CHANNEL_NUM; i++) 
	{
		if(pcbuffer[i])
			LocalFree(pcbuffer[i]);
		pcbuffer[i] = NULL;
	}
	LeaveCriticalSection(&data_cs);
	return;
}
/**
 * get_dev_stat: returns the device current state 
 *
 */
sEMGAPI int   get_dev_stat(void)
{
	return device.dev_stat;
}

/**
* get_spi_stat: returns the device current state
*
*/
sEMGAPI void   get_spi_stat(void *p)
{
	unsigned char *pp = (unsigned char *)p;
	pp[0] = spi_stat[0];
	pp[1] = spi_stat[1];
}
/**
 * get_channel_num: returns the device channel number 
 *
 */
sEMGAPI unsigned char get_channel_num(void)
{
	return device.channel_num;
}
/**
 * get_AD_rate: returns the device current AD rate 
 *
 */
sEMGAPI unsigned char get_AD_rate(void)
{
	return device.AD_rate;
}
/**
 * get_dev_stat: returns the device current timestamp 
 *
 */
sEMGAPI unsigned int  get_timestamp(void)
{
	return timestamp;
}
/**
 * get_dev_stat: returns the device lose packet number 
 *
 */
sEMGAPI unsigned int  get_losenum(void)
{
    return lose_num;
}

/**
 * get_sEMG_data: get data from internal data buffer if any
 * 
 *
 * @param[in] channel_id the data return channel' id
 * @param[in] size the size of pd
 * @param[out] pd an array to store the data
 * @return the count of data actually put into pd[]
 * @note : the function will just return one sensor_node's data
 */
sEMGAPI int   get_sEMG_data(int channel_id, unsigned int size, void *pd)
{
    int i;
	int num;//the actual count of data to read
	unsigned int header;
	struct sEMGdata *psn;
	num = 0;
	//TODO 优化掉数据同步问题
	psn = (struct sEMGdata *)pd;
	if (channel_id>=MAX_CHANNEL_NUM)
	     return num;
	EnterCriticalSection(&data_cs);
	if(pcbuffer[channel_id]==NULL)
		return num;
	if (pcbuffer[channel_id]->valid_amount) //any valid channel data
		{
			if (size < pcbuffer[channel_id]->valid_amount)
				num = size;
			else
				num = pcbuffer[channel_id]->valid_amount;
	
			header = (pcbuffer[channel_id]->header + CYCLICAL_BUFFER_SIZE - pcbuffer[channel_id]->valid_amount + 1) % CYCLICAL_BUFFER_SIZE;//计算数据指针初始位置
			pcbuffer[channel_id]->valid_amount -= num;
			
			for (i=0; i<num; i++) 
			{
				((struct sEMGdata*)pd)[i] = pcbuffer[channel_id]->data[header];
				header = (++header) % CYCLICAL_BUFFER_SIZE;
			}
		}

#ifdef DLL_DEBUG_MODE
	//OutputDebugPrintf("DEBUG_INFO | channelID = %d num = %d, valid_amount = %d\n",channel_id,num,pcbuffer[channel_id]->valid_amount);
#endif

    LeaveCriticalSection(&data_cs);
	return num;
}
