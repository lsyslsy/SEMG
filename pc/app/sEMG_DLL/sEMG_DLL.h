#ifndef _SEMG_DLL_H_
#define _SEMG_DLL_H_

#ifdef IN_WINDOWS
#include <windows.h>
#define BUILDING_DLL
#ifdef BUILDING_DLL
#define sEMGAPI extern "C" _declspec(dllexport)
#else
#define sEMGAPI extern "C" _declspec(dllimport)
#endif

#else // unix

#define sEMGAPI extern

#endif



/**
 * Open sEMG device, start the auxiliary thread.
 * @param[in] wait: if TRUE wait untill open the device,else return immediately.
 * @param[in] ip The device ip address.
 * @return TRUE if operation is successful,else return FALSE.
 */
sEMGAPI bool  sEMG_open(bool wait, const char *ip, int filter_options);

/**
 * Close SEMG device.
 */
sEMGAPI bool  sEMG_close(void);

/**
 * Reset the device, restart the auxiliary thread.
 *
 * @param[in] wait the parameter used to call wmsdas_open.
 * @return  TRUE if operation successful, else returns FALSE.
 */
sEMGAPI bool  sEMG_reset(bool wait, char *ip, int filter_options);

/**
 * Read dll info.
 * @param[in] pinfo pointer to a char buffer that has a size larger than dll_info[]
 * @return the descroptor of this dll
 */
sEMGAPI void  get_dll_info(char *pinfo);

/**
 * Clear internal data buffer.
 * @deprecated not used
 * @note this will lose all the data in the buffer
 */
sEMGAPI void  clearbuffer(void);

/**
 * @returns The device current state.
 *
 */
sEMGAPI int   get_dev_stat(void);

/**
 * @deprecated not used
 * @returns The spi current state.
 *
 */
sEMGAPI void   get_spi_stat(void *p);

/**
 * @returns the device channel number.
 *
 */
sEMGAPI unsigned char   get_channel_num(void);

/**
 *@returns the device current AD  sample rate .
 *
 */
sEMGAPI unsigned char   get_AD_rate(void);

/**
 * @returns the device current timestamp
 *
 */
sEMGAPI unsigned int    get_timestamp(void);

/**
 * @returns the device lose packet number
 *
 */
sEMGAPI unsigned int    get_losenum(void);

/**
 * Get data from internal data buffer if any.
 *
 *
 * @param[in] channel_id the data return channel' id.
 * @param[in] size the size of sEMGdata array.
 * @param[out] pd an sEMGdata array pointer to store the data.
 * @return the count of data actually put into pd[].
 * @note : the function will just return one semg channel's data
 */
sEMGAPI int   get_sEMG_data(int channel_id, unsigned int size, void *pd);

/**
 * Get sensor data from internal data buffer if any
 *
 * @param[int] sensor_num(0-3) the specified number of motion sensor board.
 * @param[int] size the size of sensorData array.包括磁、陀螺、加速度，需要*3.
 * @param[out] pd an sensorData array pointer to store the data.
 * @return the count of data acctually put into pd[].
 * @note : the function will just return one sensor board's data
 */
sEMGAPI int get_sensor_data(int sensor_num, unsigned int size, void *pd);

/**
 * set the callback function of notification
 *
 * @param[in] pfunc notification callback function pointer
 */
sEMGAPI void set_data_notify(void(*pfunc)(void));

/**
 * clear the callback function of notification and set donothing
 *
 */
sEMGAPI void reset_data_notify();

#endif
