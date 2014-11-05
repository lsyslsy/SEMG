#pragma once
#include <windows.h>


#define BUILDING_DLL

#ifdef BUILDING_DLL
#define sEMGAPI extern "C" _declspec(dllexport)
#else
#define sEMGAPI extern "C" _declspec(dllimport)
#endif


/**  
 * @brief Open SEMG device.
 * 
 */
sEMGAPI BOOL  sEMG_open(BOOL wait, char *ip, int filter_options);

/** 
 * @brief Close SEMG device
 */
sEMGAPI BOOL  sEMG_close(void);

/**
*  @brief Reset SEMG device
*/
sEMGAPI BOOL  sEMG_reset(BOOL wait, char *ip, int filter_options);

/**
*  Read dll information
*/
sEMGAPI void  get_dll_info(char *pinfo);

sEMGAPI void  clearbuffer(void);

sEMGAPI int   get_dev_stat(void);

sEMGAPI void   get_spi_stat(void *p);

sEMGAPI unsigned char   get_channel_num(void);

sEMGAPI unsigned char   get_AD_rate(void);

sEMGAPI unsigned int    get_timestamp(void);

sEMGAPI unsigned int    get_losenum(void);

sEMGAPI int   get_sEMG_data(int channel_id, unsigned int size, void *pd);


sEMGAPI void set_data_notify(void (*pfunc)(void));

sEMGAPI void reset_data_notify();

sEMGAPI void reset_data_notify();


