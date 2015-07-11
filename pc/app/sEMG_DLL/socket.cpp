#ifdef UNICODE
#undef UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef IN_WINDOWS

#include "WinSock2.h"
#pragma comment(lib,"ws2_32.lib")
#include <windows.h>
#include <setupapi.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#define closesocket close
#define ioctlsocket ioctl
#endif

#include <assert.h>
//#include <strsafe.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include "Butterworth.h"
#include "RLS.h"
#include "FOURIER.h"
#include "socket.h"
#include "Baseline_corrector.h"
#include "semg_debug.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <stdint.h>

using namespace std::chrono;

#ifdef IN_WINDOWS
SOCKET tcp_socket;
#else
int tcp_socket;
#endif
struct sockaddr_in tcpAddr;
unsigned int BufLen = 1024 * 32;
//char IP[] = "10.13.89.32";//pc虚拟机地址
//char IP[] = "192.168.137.101";//开发板无线地址（连接PC热点）
//char IP[] = "192.168.0.101";//开发板无线地址（连接HY路由器）
//char * IP;// [] = "10.13.89.25";//开发板有线地址
const int PORT = 5000;
fd_set  fdread;
fd_set  fdwrite;
struct timeval con_timeout = { 2, 0 }; //2 sec
struct timeval sen_timeout = { 1, 0 }; //1 sec
struct timeval rev_timeout = { 1, 0 };

unsigned int timestamp = 0;
unsigned int old_timestamp = 0;
unsigned int lose_num = 0;
bool inited = false;
unsigned char spi_stat[2];//暂时用来反馈SPI用

std::mutex data_mutex;			/* for pcbuffer use */
struct cyc_buffer *pcbuffer[MAX_CHANNEL_NUM] = { nullptr };	/* pointer of internal cyclical data buffer */

RLS_PARM RlsFilters[MAX_CHANNEL_NUM];
Butterworth_4order BwFilters[MAX_CHANNEL_NUM];
double BaseLine_bias[MAX_CHANNEL_NUM] = { 0 };
//int baseline_option = 0;	//<! 基线漂移的选项
int Filter_Options = 0;		//<! 滤波选项，包括50Hz和基线漂移
struct dev_info device;
void(*notify_data)(void) = NULL;
void do_nothing(){}


/* start_comu_thread: start the communication thread */
void start_comu_thread(unsigned int *tid, struct thread_args *args)
{
	//HANDLE ht;
	args->threadrun = true;
	if (notify_data == NULL)
		notify_data = do_nothing;
	//ht = (HANDLE)_beginthreadex(NULL, 0, comu_thread_proc, args, 0, tid);
	comu_thread_proc(args);
	//thread
	DebugInfo("start comu thread\n"); //with handle : %d\n"),ht);
	//return ht;
}

// 1个问题，不进行join就没法确定该线程已经down了
/* stop_comu_thread: stop the communication thread */
void stop_comu_thread(int ht, struct thread_args *args)
{
	args->threadrun = false;

	std::this_thread::sleep_for(seconds{ 1 });		/* wait for a while, the comu_thread may be frozen in read_data() .etc */
	//CloseHandle(ht);
	notify_data = do_nothing;

}

/*
* comu_thread_proc: communication thread function
* This thread function handles most of the communication thing
*/
void comu_thread_proc(void *pargs)//(struct thread_args *args)
{
	//struct dev_info dinfo;	/* device info */
	thread_args *args = (thread_args *)pargs;
	init_dll();
	while (args->threadrun) {
		device.dev_stat = dev_START;
		protocol_handler(&device, &(args->threadrun));
	};
	uninit();
	//return 0;
}


/* init_dll: initialize the dll */
void init_dll(void)
{
	int i;

	//init the data buffer
	data_mutex.lock();
	for (i = 0; i < MAX_CHANNEL_NUM; i++) {
		pcbuffer[i] = (struct cyc_buffer *)malloc(sizeof(struct cyc_buffer));
	}
	data_mutex.unlock();

	device.dev_stat = dev_NONE;
	device.version = 0;
	device.AD_rate = 0;
	device.channel_num = 0;
	inited = true;

	DebugInfo("DLL initilization complete.\n");

	return;
}

/* uninit: uninitialize everything */
void uninit(void)
{
	int i;
	data_mutex.lock();
	for (i = 0; i < MAX_CHANNEL_NUM; i++) {
		if (pcbuffer[i])
			free(pcbuffer[i]);
		pcbuffer[i] = NULL;	/* no data right now */
	}
	data_mutex.unlock();
	inited = false;
	return;
}

/* protocol_handler: handle the app protocol */
bool protocol_handler(struct dev_info *pdi, bool *prun)
{
	int i;
	struct protocol_stat stat;
	bool isok = true;

	stat.cmd_stat = PSTAT_NO_DEVICE;
	//stat.inquire = 0;
	stat.error = SOCKET_OK;

	assert(pdi != NULL);
	assert(prun != NULL);
	if (pdi->dev_stat == dev_START) {
		stat.cmd_stat = PSTAT_DEVICE_OPEN;
	} else {
		DebugInfo("init??\n");
		return false;
	}
	//32KB
	stat.pdata = (unsigned char *)malloc(MAX_TURN_BYTE);//here!分配大小
	assert(stat.pdata != NULL);
	data_mutex.lock();
	for (i = 0; i < MAX_CHANNEL_NUM; i++) {
		pcbuffer[i]->header = -1;
		pcbuffer[i]->valid_amount = 0;
		pcbuffer[i]->channel_id = i;

		RLS::filter_init(&RlsFilters[i]);
		BaseLine_bias[i] = 0;
		Butterworth::filter_init(&BwFilters[i]);
	}
	data_mutex.unlock();
	stat.cmd_stat = PSTAT_INIT_SOCKET;
	DebugInfo("Everything is ok. Ready to go protocols.");
	//unsigned long temp = 0 ;
	while (*prun)//when thread should run
	{
		switch (stat.cmd_stat) {
		case PSTAT_INIT_SOCKET:
			isok = init_socket(pdi, &stat);
			break;
		case PSTAT_SHAKEHAND:
			isok = shakehand(pdi, &stat);
			break;
		case PSTAT_DATA:
			//isok = shakehand_sec(pdi, &stat);
			isok = update_data(pdi, &stat);
			break;
		default:
			isok = false;
			break;
		}

		if (!isok && !error_handler(&stat))
			break;
	}

	closesocket(tcp_socket);
#ifdef IN_WINDOWS
	WSACleanup();   //释放套接字资源;
#endif
	if (stat.pdata)
		free(stat.pdata);
	return isok;
}

// 缺少很多的错误处理
/* init_socket: init the socket */
bool init_socket(struct dev_info *pdi, struct protocol_stat *pstat)
{
#ifdef IN_WINDOWS
	WSADATA wsd;//Windows异步套接字服务
#endif
	//int ret;
	long int Addr;
	int Port;
	int snd_size;
	int rcv_size;
#ifdef IN_WINDOWS
	int optlen = sizeof(int);
#else
	unsigned int optlen = sizeof(int);
#endif

	Addr = inet_addr(device.ip);//点分十进制字符串转成ulong
	Port = PORT;
#ifdef IN_WINDOWS
	WSAStartup(MAKEWORD(2, 2), &wsd);//Winsock 2.2
#endif
	tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket < 0) {
		DebugError("Create Socket Failed!\n");
		pstat->error = SOCKET_INIT_ERROR;
		return false;
	}
	tcpAddr.sin_family = AF_INET;
	tcpAddr.sin_port = htons(Port);
	tcpAddr.sin_addr.s_addr = Addr;
	// 接收缓冲区
	int nRecvBuf = BufLen;//设置为32K
	setsockopt(tcp_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, optlen);
	//发送缓冲区
	int nSendBuf = BufLen;//设置为32K
	setsockopt(tcp_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, optlen);

	//int nNetTimeout = 1000; //设置阻塞超时时间为1秒
	//发送时限
	//setsockopt(tcp_socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&nNetTimeout,optlen);
	//接收时限
	//setsockopt(tcp_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&nNetTimeout,optlen);

	getsockopt(tcp_socket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, &optlen);
	getsockopt(tcp_socket, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size, &optlen);
	DebugInfo(" 发送缓冲区原始大小为: %d 字节\n", snd_size);
	DebugInfo(" 接收缓冲区原始大小为: %d 字节\n", rcv_size);

	unsigned long mode = 1;//设置为非阻塞方式，0 为阻塞
	ioctlsocket(tcp_socket, FIONBIO, &mode);//把套接字设成非阻塞的

	//error process
	//WSACleanup( );
	pstat->cmd_stat = PSTAT_SHAKEHAND;
	return true;
}
/* shakehand: shake hand with arm */
bool shakehand(struct dev_info *pdi, struct protocol_stat *pstat)
{
	//int ret;
	char cmd;
	int length;
#ifdef IN_WINDOWS
	connect(tcp_socket, (LPSOCKADDR)&tcpAddr, sizeof(tcpAddr));
#else
	connect(tcp_socket, (struct sockaddr *)&tcpAddr, sizeof(tcpAddr));
#endif
	if (Is_connect_ready()) {
		//connect(tcp_socket,(LPSOCKADDR)&tcpAddr,sizeof(tcpAddr));
		DebugInfo("connect is OK!\n");
		//pstat->cmd_stat= PSTAT_DATA;
		pdi->dev_stat = dev_CONNECT;
	} else {
		DebugError("connect error!\n");
		pdi->dev_stat = dev_UNCONNECT;
		pstat->error = SOCKET_CONNECT_TIMEOUT;
		return false;
	}
	if (Is_send_ready() == false) {
		pstat->error = SOCKET_DATA_TIMEOUT;
		return false;
	}
	cmd = 'H';
	/* handshake step 1: 'h'<-->'h' */
	length = send(tcp_socket, &cmd, 1, 0);
	if (length > 0) {
		//ret = select(tcp_socket+1, &fdread, NULL, NULL, &rev_timeout);
		if (Is_recv_ready() == false) {
			pstat->error = SOCKET_DATA_TIMEOUT;
			return false;
		}
		//是否要加waitAll标志
		length = recv(tcp_socket, (char*)pstat->pdata, BufLen, 0);
		if (length > 0) {
			//int i;
			unsigned char *pd = pstat->pdata;
			printf("sec_recvdata:%c,%d\n", *pd, length);
			if (*pd != 'h') {
				DebugWarn("data wrong：header check failed!\n");
				pstat->error = SOCKET_DATA_WRONG;
				return false;
			}
			/* information about root node */
			pd += 1;
			pdi->version = (*pd << 8) + *(pd + 1);
			pdi->channel_num = *(pd + 2);
			pdi->AD_rate = *(pd + 3);
			DebugInfo("version is %d\n", pdi->version);
			pstat->cmd_stat = PSTAT_DATA;
		} else {
			printf("receive error!\n");
			pstat->error = SOCKET_DATA_TIMEOUT;//用这个错误不准确
			return false;
		}
	} else /* error occured */
	{
		printf("send error!\n");
		pstat->error = SOCKET_DATA_TIMEOUT;
		return false;
	}
	return true;
}

//}
/* update_data: update data using wifi packet */
bool update_data(struct dev_info *pdi, struct protocol_stat *pstat){

	bool ret = false;
	char cmd;

	int length;
	unsigned int temp_packet_size;
	unsigned int packet_size;

	assert(pdi != NULL);
	assert(pstat != NULL);
	if (Is_send_ready() == false) {
		pstat->error = SOCKET_DATA_TIMEOUT;
		return false;
	}
	cmd = 'D';
	length = send(tcp_socket, &cmd, 1, 0);
	//Sleep(90);//wait a while
	if (length > 0) {
		if (Is_recv_ready() == false) {
			pstat->error = SOCKET_DATA_TIMEOUT;
			return false;
		}

		double totaltime;
		auto start = high_resolution_clock::now();
		length = recv(tcp_socket, (char*)pstat->pdata, BufLen, 0);
		unsigned char *pd = pstat->pdata;
		//printf("recv length = %d\n",length);
		if (*pd != 'd') {
			printf("data wrong!\n");
			pstat->error = SOCKET_DATA_WRONG;
			return false;
		}
		pd += 1;
		temp_packet_size = length;
		packet_size = (*pd << 8) + *(pd + 1);
		pd += 2;
		timestamp = (*pd << 8) + *(pd + 1);
		if (old_timestamp != 0)
			lose_num += timestamp - old_timestamp - 1;
		old_timestamp = timestamp;
		//if ((timestamp - old_timestamp > 1))
		//printf("%d\n",timestamp);
		//printf("%d,%d,%d,%d,%d\n",*(pd+2),*(pd+3),*(pd+4),*(pd+5));
		while (temp_packet_size < packet_size)//still got data to be read
		{
			if (Is_recv_ready() == false) {
				pstat->error = SOCKET_DATA_TIMEOUT;
				return false;
			}
			length = recv(tcp_socket, (char*)(pstat->pdata + temp_packet_size), BufLen, 0);
			//printf("recv length = %d\n",length);
			temp_packet_size += length;
		}
		auto finish = high_resolution_clock::now();
		totaltime = duration_cast<microseconds>(finish - start).count() / 1000.0;
		DebugInfo("send used:%f ms\n", totaltime);
	} else {  /* error occured */
		printf("send error!\n");
		pstat->error = SOCKET_DATA_TIMEOUT;
		return false;
	}
	return update_cbuffer(pdi, pstat);
}
/* update_cbuffer: update data into buffer */
bool update_cbuffer(struct dev_info *pdi, struct protocol_stat *pstat){
	int i;
	bool ret = false;
	unsigned char *pdata = pstat->pdata;
	unsigned int size;
	//unsigned char data_num;
	//unsigned char index_temp;

	assert(pdi != NULL);

	pdata += 1;
	size = (*pdata << 8) + *(pdata + 1);
	size -= 7;


	data_mutex.lock();
	pdata += 4;
	spi_stat[0] = pdata[0];
	spi_stat[1] = pdata[1];
	pdata += 2;
	// TODO size不够也会是一个坑
	while (size > 0) {
		switch (*pdata) {

		case 0x11: // sEMG 数据
			pdata += 1;
			//use a flexible way to match the channel
			if (*pdata >= pdi->channel_num) {
				DebugError("data parse 0x11 error\n");
				goto err;
			}
			i = *pdata;
			//	printf("addr num:%d, id : %d\n",i, pcbuffer[i]->channel_id);
			pdata += 1;
			pdata += 1;//state
			size -= 3;
			if (size < 200) {
				DebugError("sEMG size < 200\n");
				goto err;
			}
			parse_data(pdata, pcbuffer[i], i);
			pdata += 200;
			size -= 200;
			break;
		case 0x12: // motion sensor 数据
			if (size < 92) {
				DebugError("motion sensor size < 92\n");
				goto err;

			}
			pdata += 92;
			size -= 92;
			break;
		case 0xED:
			//printf("parse_data finish!\n");//解包结束
			//pdata += 1;
			size -= 1;
			break;
		default:
			DebugError("padta error!size:%d\n", size);
			pdata += 1;
			size -= 1;
			timestamp--;//减一有问题
			//遇到不能解释的字节就立刻返回,时间戳减1，可视作该包丢失
			goto err;
		}
	}
	data_mutex.unlock();
	if (notify_data != NULL)
		notify_data();
	return true;
err:
	pstat->error = SOCKET_DATA_WRONG;
	data_mutex.unlock();
	return false;
}
/* parse_data: parse datapacket*/
void parse_data(unsigned char *pdata, struct cyc_buffer *pcb, int num){
	int i;
	if (pdata == NULL)
		return;
	pcb->header = (++(pcb->header)) % CYCLICAL_BUFFER_SIZE;
	if ((++(pcb->valid_amount)) > CYCLICAL_BUFFER_SIZE) {
		pcb->valid_amount = CYCLICAL_BUFFER_SIZE;
		//DebugWarn("data buffer overflow\n");
	}
	for (i = 0; i < POINT_NUM; i++) {
		pcb->raw_data[pcb->header].point[i] = ((int16_t)((*pdata << 8) + *(pdata + 1))) * DATASCALE / (float)32768;
		//pcb->data[pcb->header].point[i] = 1;
		pdata += 2;
		//printf("point = %f\n", pcb->data[pcb->header].point[i]);
	}

	if ((Filter_Options & 0x00ff) == FILTER_BUTTERWORTH)
		Butterworth::filter(&BwFilters[num], pcb->raw_data[pcb->header].point, pcb->data[pcb->header].point, POINT_NUM);
	else if ((Filter_Options & 0x00ff) == FILTER_RLS)
		RLS::filter(&RlsFilters[num], pcb->raw_data[pcb->header].point, pcb->data[pcb->header].point, POINT_NUM);
	else if ((Filter_Options & 0x00ff) == FILTER_FOURIER)
		FOURIER::filter(NULL, pcb->raw_data[pcb->header].point, pcb->data[pcb->header].point, POINT_NUM);
	else// if FILTER_NONE do nothing
		memcpy(pcb->data[pcb->header].point, pcb->raw_data[pcb->header].point, POINT_NUM*sizeof(double));
	if ((Filter_Options & 0xff00) == FILTER_BASELINE_YES)
		Baseline_corrector::correct(BaseLine_bias + num, pcb->data[pcb->header].point, pcb->data[pcb->header].point, POINT_NUM);


#ifdef DLL_DEBUG_MODE
	//printf("point = %f", pcb->data[pcb->header].point[i]);
	//OutputDebugPrintf("DEBUG_INFO |valid_amount = %d\n",pcbuffer[i]->valid_amount);
#endif
}


inline bool Is_connect_ready(void)
{
	int ret;
	FD_ZERO(&fdread);
	FD_SET(tcp_socket, &fdread);
	FD_ZERO(&fdwrite);
	FD_SET(tcp_socket, &fdwrite);
	con_timeout.tv_sec = 2;
	con_timeout.tv_usec = 0;
	// Note: linux会改变该con_timeout值,而且每次select以后描述符集合可能会变化
	ret = select(tcp_socket + 1, NULL, &fdwrite, NULL, &con_timeout);
	if (ret <= 0) {
		printf("connect timeout!\n");
		return false;
	}
	return true;

}
/* Is_send_ready: using this func to decide whether the socket send buff is ready*/
inline bool Is_send_ready(void)
{
	int ret;
	FD_ZERO(&fdread);
	FD_SET(tcp_socket, &fdread);
	FD_ZERO(&fdwrite);
	FD_SET(tcp_socket, &fdwrite);
	sen_timeout.tv_sec = 1;
	sen_timeout.tv_usec = 0;
	// Note: linux会改变该con_timeout值,而且每次select以后描述符集合可能会变化
	ret = select(tcp_socket + 1, NULL, &fdwrite, NULL, &sen_timeout);
	if (ret <= 0) {
		printf("send timeout!\n");
		return false;
	}
	return true;
}
/* Is_recv_ready: using this func to decide whether the socket receive buff is ready*/
inline bool Is_recv_ready(void)
{
	int ret;
	FD_ZERO(&fdread);
	FD_SET(tcp_socket, &fdread);
	FD_ZERO(&fdwrite);
	FD_SET(tcp_socket, &fdwrite);
	rev_timeout.tv_sec = 1;
	rev_timeout.tv_usec = 0;
	// Note: linux会改变该con_timeout值,而且每次select以后描述符集合可能会变化
	ret = select(tcp_socket + 1, &fdread, NULL, NULL, &rev_timeout);
	if (ret <= 0) {
		printf("receive timeout!\n");
		return false;
	}
	return true;
}

/* error_handler: handle the errors occured */
bool error_handler(struct protocol_stat *pstat)
{
	//if (pstat->erro == ERROR_ACCESS_DENIED) /* too many entries */
	//return false;
	/* socket data timeout */
	/*if (pstat->error == SOCKET_CONNECT_TIMEOUT)
	{
	pstat->cmd_stat = PSTAT_SHAKEHAND;
	Sleep(1000);
	return true;
	}*/
	//TODO ddd
	return false;//templly not deal with,反正上层会重启的 而且目前错误状态机制可能不完备

	if (pstat->error == SOCKET_DATA_TIMEOUT || pstat->error == SOCKET_CONNECT_TIMEOUT
		|| pstat->error == SOCKET_DATA_WRONG) {
		/*pstat->inquire++;
		if (pstat->inquire >= TIMES_TO_INQUIRE)
		{
		pstat->cmd_stat = PSTAT_SHAKEHAND;
		pstat->inquire = 0;
		}*/
		printf("try to init socket again!\n");
		pstat->cmd_stat = PSTAT_INIT_SOCKET;
		closesocket(tcp_socket);
#ifdef IN_WINDOWS
		WSACleanup();   //释放套接字资源;
#endif
		std::this_thread::sleep_for(seconds{ 2 });
		return true;
	}
	if (pstat->error == SOCKET_INIT_ERROR) {
		return false;
	}
	return true;
}