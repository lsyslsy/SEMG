#ifdef UNICODE
#undef UNICODE
#endif

#include <stdio.h>
#include "WinSock2.h"
#pragma comment(lib,"ws2_32.lib")
#include <windows.h>
#include <process.h>
#include <assert.h>
#include <setupapi.h>
#include <strsafe.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include "Butterworth.h"
#include "RLS.h"
#include "FOURIER.h"
#include "socket.h"
#include "Baseline_corrector.h"
#include "semg_debug.h"

SOCKET tcp_socket;
SOCKADDR_IN tcpAddr;
unsigned int BufLen=1024*32;
//char IP[] = "10.13.89.32";//pc虚拟机地址
//char IP[] = "192.168.137.101";//开发板无线地址（连接PC热点）
//char IP[] = "192.168.0.101";//开发板无线地址（连接HY路由器）
//char * IP;// [] = "10.13.89.25";//开发板有线地址
int PORT = 5000;
fd_set  fdread;
fd_set  fdwrite;
struct timeval con_timeout = {2,0}; //2 sec
struct timeval sen_timeout = {1,0}; //1 sec
struct timeval rev_timeout = {1,0};

unsigned int timestamp = 0;
unsigned int old_timestamp = 0;
unsigned int lose_num = 0;
BOOL inited;
unsigned char spi_stat[2];//暂时用来反馈SPI用

CRITICAL_SECTION data_cs;			/* for pcbuffer use */
struct cyc_buffer *pcbuffer[MAX_CHANNEL_NUM];	/* pointer of internal cyclical data buffer */ 

RLS_PARM RlsFilters[MAX_CHANNEL_NUM];
Butterworth_4order BwFilters[MAX_CHANNEL_NUM];
double BaseLine_bias[MAX_CHANNEL_NUM] = { 0 };
//int baseline_option = 0;	//<! 基线漂移的选项
int Filter_Options = 0;		//<! 滤波选项，包括50Hz和基线漂移
struct dev_info device;
void (*notify_data)(void);
void do_nothing(){}
/* start_comu_thread: start the communication thread */
HANDLE start_comu_thread(unsigned int *tid, struct thread_args *args)
{
	HANDLE ht;
	args->threadrun = TRUE;
	notify_data = do_nothing;
	InitializeCriticalSection(&(args->cs));
	ht = (HANDLE)_beginthreadex(NULL, 0, comu_thread_proc, args, 0, tid);
	DebugInfo(TEXT("start comu thread with handle:%d\n"),ht);
	return ht;
}

/* stop_comu_thread: stop the communication thread */
void stop_comu_thread(HANDLE ht, struct thread_args *args)
{
	args->threadrun = FALSE;
	
	Sleep(1000);		/* wait for a while, the comu_thread may be frozen in read_data() .etc */
	CloseHandle(ht);
	notify_data = do_nothing;
	DeleteCriticalSection(&(args->cs));
	
}

/*
* comu_thread_proc: communication thread function
* This thread function handles most of the communication thing
*/
UINT WINAPI comu_thread_proc(void *pargs)//(struct thread_args *args)
{
	//struct dev_info dinfo;	/* device info */
	thread_args *args = (thread_args *)pargs;
	//printf("%d\n",args->threadrun);
	while (args->threadrun) 
	{
		device.dev_stat = dev_START;
		protocol_handler(&device, &(args->threadrun));
		/*if (inited)
			uninit();
		init_dll();*/
	}
	return 0;
}


/* init_dll: initialize the dll */
void init_dll(void)
{
	int i;

	if (!inited) 
	{
		InitializeCriticalSection(&data_cs);
	}
	//init the data buffer
	EnterCriticalSection(&data_cs);	
	for (i=0;i<MAX_CHANNEL_NUM;i++)
	{
		if (pcbuffer[i])
			LocalFree(pcbuffer[i]);
		pcbuffer[i] = NULL;	// no data right now
	}
	LeaveCriticalSection(&data_cs);

	if (!inited) 
	{
		device.dev_stat = dev_NONE;
		notify_data = NULL;
	}
	device.version = 0;
	device.AD_rate = 0;
	device.channel_num = 0;
	inited = TRUE;

	DebugInfo(TEXT("DLL initilization complete.\n"));

	return;
}

/* uninit: uninitialize everything */
void uninit(void)
{
	closesocket(tcp_socket);
	WSACleanup();   //释放套接字资源;

	int i;
	EnterCriticalSection(&data_cs);
	for (i = 0; i<MAX_CHANNEL_NUM; i++)
	{
		if (pcbuffer[i])
			LocalFree(pcbuffer[i]);
		pcbuffer[i] = NULL;	/* no data right now */
	}
	LeaveCriticalSection(&data_cs);
	DeleteCriticalSection(&data_cs);
	inited = FALSE;
	notify_data = do_nothing;
	return;
}

/* protocol_handler: handle the app protocol */
BOOL protocol_handler(struct dev_info *pdi, BOOL *prun)
{
	int i;
	struct protocol_stat stat;
	BOOL isok = TRUE;

	stat.cmd_stat = PSTAT_NO_DEVICE;
	//stat.inquire = 0;
	stat.error = SOCKET_OK;

	assert(pdi != NULL);
	assert(prun != NULL);
	if (pdi->dev_stat == dev_START) 
	{
		stat.cmd_stat = PSTAT_DEVICE_OPEN;
	} 
	else 
	{
		DebugInfo(TEXT("init??\n"));
		return FALSE;
	}
	//32KB
	stat.pdata = (unsigned char *)LocalAlloc(LMEM_FIXED,MAX_TURN_BYTE);//here!分配大小
	assert(stat.pdata != NULL);
	EnterCriticalSection(&data_cs);
	for (i=0;i<MAX_CHANNEL_NUM;i++)
	{
		pcbuffer[i] = (struct cyc_buffer *)LocalAlloc(LMEM_FIXED, sizeof(struct cyc_buffer));
		pcbuffer[i]->header = -1;
		pcbuffer[i]->valid_amount = 0;
		pcbuffer[i]->channel_id = i;

		RLS::filter_init(&RlsFilters[i]);
		BaseLine_bias[i] = 0;
		Butterworth::filter_init(&BwFilters[i]);
	}
	LeaveCriticalSection(&data_cs);
	stat.cmd_stat = PSTAT_INIT_SOCKET;
	DebugInfo(TEXT("Everything is ok. Ready to go protocols."));
	//unsigned long temp = 0 ;
	while (*prun)//when thread should run
	{    
		switch (stat.cmd_stat) 
		{
		case PSTAT_INIT_SOCKET:
			isok = init_socket(pdi, &stat);
			break; 
		case PSTAT_SHAKEHAND:
			isok = shakehand(pdi, &stat);			
			break;
		case PSTAT_DATA:
			//isok = shakehand_sec(pdi, &stat);		
			isok = update_data(pdi, &stat);
			//temp++;
			//printf("totalturn:%ld\n",temp);
			//Sleep(500);
			break;
		default:
			isok = FALSE;
			break;
		}
		//		if (WAIT_OBJECT_0 == WaitForSingleObject(g_eventLowerUPTRate, 0))
		//		{
		//			stat.cmd_stat = PSTAT_CHANGE_UPDATE_RATE;
		//			root_stat.sensor_update_rate = 30;
		//		}

		if (!isok && !error_handler(&stat))
			break;
	}
	if (stat.pdata)
		LocalFree(stat.pdata);
	return isok;
}

/* init_socket: init the socket */
BOOL init_socket(struct dev_info *pdi,struct protocol_stat *pstat)
{
	WSADATA wsd;//Windows异步套接字服务
	//int ret;
	long int Addr;
	int Port;
	int snd_size;
	int rcv_size;
	int optlen = sizeof(int);
	//if(argc!=3){printf("Usage:%s [<IP> <Port>]\n",argv[0]);return 0;}
	/* if(argc!=2)
	{
	printf("Usage:%s [<IP>]\n",argv[0]);
	return FALSE;
	}*/
	//Addr=inet_addr(argv[1]);
	Addr = inet_addr(device.ip);//点分十进制字符串转成ulong
	//Port=atoi(argv[2]);
	Port = PORT;
	WSAStartup(MAKEWORD(2,2),&wsd);//Winsock 2.2
	tcp_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (tcp_socket <0)
	{
		DebugError(TEXT("Create Socket Failed!\n"));
		pstat->error = SOCKET_INIT_ERROR;
		return FALSE;
	}
	tcpAddr.sin_family=AF_INET;
	tcpAddr.sin_port=htons(Port);
	tcpAddr.sin_addr.s_addr=Addr;
	// 接收缓冲区
	int nRecvBuf= BufLen;//设置为32K
	setsockopt(tcp_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,optlen);
	//发送缓冲区
	int nSendBuf= BufLen;//设置为32K
	setsockopt(tcp_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,optlen);

	//int nNetTimeout = 1000; //设置阻塞超时时间为1秒
	//发送时限
	//setsockopt(tcp_socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&nNetTimeout,optlen);
	//接收时限
	//setsockopt(tcp_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&nNetTimeout,optlen);

	getsockopt(tcp_socket, SOL_SOCKET, SO_SNDBUF,(char *)&snd_size, &optlen);
	getsockopt(tcp_socket, SOL_SOCKET, SO_RCVBUF,(char *)&rcv_size, &optlen);
	DebugInfo(TEXT(" 发送缓冲区原始大小为: %d 字节\n"),snd_size); 
	DebugInfo(TEXT(" 接收缓冲区原始大小为: %d 字节\n"),rcv_size);

	ULONG mode = 1;//设置为非阻塞方式，0 为阻塞
	ioctlsocket(tcp_socket,FIONBIO,&mode);//把套接字设成非阻塞的

	FD_ZERO(&fdread);
	FD_SET(tcp_socket,&fdread);
	FD_ZERO(&fdwrite);
	FD_SET(tcp_socket,&fdwrite);
	//error process
    //WSACleanup( );
	pstat->cmd_stat = PSTAT_SHAKEHAND;
	return TRUE;
}
/* shakehand: shake hand with arm */
BOOL shakehand(struct dev_info *pdi, struct protocol_stat *pstat)
{
	//int ret;
	char cmd;
	int length;
	connect(tcp_socket,(LPSOCKADDR)&tcpAddr,sizeof(tcpAddr));
	if (Is_connect_ready())
	{
		//connect(tcp_socket,(LPSOCKADDR)&tcpAddr,sizeof(tcpAddr));
		DebugInfo(TEXT("connect is OK!\n"));
		//pstat->cmd_stat= PSTAT_DATA;
		pdi->dev_stat = dev_CONNECT;
	}
	else 
	{
		DebugError(("connect error!\n"));
		pdi->dev_stat = dev_UNCONNECT;	
		pstat->error = SOCKET_CONNECT_TIMEOUT;
		return FALSE;
	}
	if (Is_send_ready() == FALSE)
	{
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
	}
	cmd = 'H';
	/* handshake step 1: 'h'<-->'h' */
	length = send(tcp_socket,&cmd,1,0);
	if (length > 0)
	{
		//ret = select(tcp_socket+1, &fdread, NULL, NULL, &rev_timeout);
		if (Is_recv_ready() == FALSE)
		{
			pstat->error = SOCKET_DATA_TIMEOUT;
			return FALSE;
		}
		//是否要加waitAll标志
		length= recv(tcp_socket,(char*)pstat->pdata,BufLen,0);
		if (length > 0) 
		{
			//int i;
			unsigned char *pd = pstat->pdata;
			printf("sec_recvdata:%c,%d\n",*pd,length);
			if (*pd != 'h' )
			{
				DebugWarn(TEXT("data wrong：header check failed!\n"));
				pstat->error = SOCKET_DATA_WRONG;
				return FALSE;
			}
			/* information about root node */
			pd += 1;
			pdi->version = (*pd << 8) + *(pd+1);
			pdi->channel_num = *(pd+2);
			pdi->AD_rate = *(pd+3);
			DebugInfo(TEXT("version is %d\n"),pdi->version);
			pstat->cmd_stat= PSTAT_DATA;
		}
		else
		{
			printf("receive error!\n");
			pstat->error = SOCKET_DATA_TIMEOUT;//用这个错误不准确
			return FALSE;
		}
	} 
	else /* error occured */
	{	
		printf("send error!\n");
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
	}
	return TRUE;
}

//BOOL shakehand_sec(struct dev_info *pdi, struct protocol_stat *pstat)
//{
//     int length; 
//	 //int ret;
//	 //ret = select(tcp_socket+1, NULL, &fdwrite, NULL, &sen_timeout);
//	 if (Is_send_ready() == FALSE)
//	 {
//		 pstat->error = SOCKET_DATA_TIMEOUT;
//			return FALSE;
//	 }
//	*(pstat->pdata) = 'H';
//	/* handshake step 1: 'h'<-->'h' */
//	length = send(tcp_socket,(char*)pstat->pdata,1,0);
//	if (length >= 0)
//	{
//		//ret = select(tcp_socket+1, &fdread, NULL, NULL, &rev_timeout);
//		if (Is_recv_ready() == FALSE)
//		{
//			pstat->error = SOCKET_DATA_TIMEOUT;
//				return FALSE;
//		}
//		length= recv(tcp_socket,(char*)pstat->pdata,BufLen,0);
//        if (length >= 0) 
//		{
//			printf("sec_recvdata:%c,%d\n",*(pstat->pdata),length);
//			//pstat->cmd_stat= PSTAT_DATA;
//		 }
//		else
//		{
//			printf("receive error!\n");
//			pstat->error = SOCKET_DATA_TIMEOUT;
//			return FALSE;
//		}
//	} 
//	else /* error occured */
//	{	
//	    printf("send error!\n");
//		pstat->error = SOCKET_DATA_TIMEOUT;
//		return FALSE;
//	}
//	return TRUE;
//
//}
/* update_data: update data using wifi packet */
BOOL update_data(struct dev_info *pdi, struct protocol_stat *pstat){

	BOOL ret = FALSE;
	char cmd;
	
	int length;
	unsigned int temp_packet_size;
	unsigned int packet_size;

	assert(pdi != NULL);
	assert(pstat != NULL);
	if (Is_send_ready() == FALSE)
	{
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
	}
	cmd = 'D';
	length = send(tcp_socket,&cmd,1,0);
	//Sleep(90);//wait a while
	if (length > 0)
	{
		if (Is_recv_ready() == FALSE){
			pstat->error = SOCKET_DATA_TIMEOUT;
			return FALSE;
		}
		clock_t start, finish;
		double totaltime;
		start = clock();
		length = recv(tcp_socket,(char*)pstat->pdata,BufLen,0);
		unsigned char *pd = pstat->pdata;
		//printf("recv length = %d\n",length);
		if (*pd != 'd' )
		{
			printf("data wrong!\n");
			pstat->error = SOCKET_DATA_WRONG;
			return FALSE;
		}
		pd += 1;
		temp_packet_size = length;
		packet_size = (*pd << 8) + *(pd+1);
		pd += 2;
		timestamp = (*pd << 8) + *(pd+1);
		if ( old_timestamp != 0)
			lose_num += timestamp - old_timestamp - 1;
		old_timestamp = timestamp;
		//if ((timestamp - old_timestamp > 1))
		//printf("%d\n",timestamp);
		//printf("%d,%d,%d,%d,%d\n",*(pd+2),*(pd+3),*(pd+4),*(pd+5));
		while(temp_packet_size < packet_size)//still got data to be read
		{	
			if (Is_recv_ready() == FALSE){
				pstat->error = SOCKET_DATA_TIMEOUT;
				return FALSE;
			}
			length = recv(tcp_socket,(char*)(pstat->pdata + temp_packet_size),BufLen,0);
			//printf("recv length = %d\n",length);
			temp_packet_size += length;
		}
		finish = clock();
		totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
		OutputDebugPrintf("send used:%f s\n", totaltime);
		/*//回应帧，暂时取消
		if (Is_send_ready() == FALSE)
		{
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
		}
		cmd = 'A'; 
		length = send(tcp_socket,&cmd,1,0);
		if (length < 0)
		{	
		printf("send error!\n");
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
		}
		*/
	}
	else /* error occured */
	{	
		printf("send error!\n");
		pstat->error = SOCKET_DATA_TIMEOUT;
		return FALSE;
	}	
	update_cbuffer(pdi, pstat);
	return TRUE;
}
/* update_cbuffer: update data into buffer */
void update_cbuffer(struct dev_info *pdi, struct protocol_stat *pstat){
	int i;
	BOOL ret = FALSE;
	unsigned char *pdata = pstat->pdata;
	unsigned int size;
	//unsigned char data_num;
	//unsigned char index_temp;

	assert(pdi != NULL);
	assert(pstat != NULL);
	assert(pstat->pdata != NULL);

	pdata +=1;
	size = (*pdata << 8) + *(pdata+1);
	size -= 7;
	

	EnterCriticalSection(&data_cs);	
	pdata += 4;
	spi_stat[0] = pdata[0];
	spi_stat[1] = pdata[1];
	pdata += 2;
	while (size > 0) 
	{
		switch (*pdata)
		{

		case 0x11:
			//if(size<SIZE_PACKET_FLL_DATA)
			//{
			//LeaveCriticalSection(&data_cs);
			//return;
			//}
			pdata += 1;		
			//use a flexible way to match the channel
			for (i=0; i < pdi->channel_num; i++)
			if ( pcbuffer[i]->channel_id == *pdata)//find the channel id
			{
				//printf("channel_id:%d\n",i);
				break;
			}
			//	printf("addr num:%d, id : %d\n",i, pcbuffer[i]->channel_id);
			pdata += 1;
			pdata += 1;//state
			size -= 3;
			parse_data(pdata, pcbuffer[i], pcbuffer[i]->channel_id);
			pdata += 200;
			size -= 200;
			break;
		case 0xED:
			//printf("parse_data finish!\n");//解包结束
			//pdata += 1;
			size -= 1;						
			break;
		default:
			printf("padta error!size:%d\n",size);
			pdata += 1;
			size -= 1;
			timestamp --;//减一有问题
			//遇到不能解释的字节就立刻返回,时间戳减1，可视作该包丢失
			LeaveCriticalSection(&data_cs);
			return;
			break;
		}
	}
	LeaveCriticalSection(&data_cs);
	if(notify_data != NULL)
	notify_data();
}
/* parse_data: parse datapacket*/
void parse_data(unsigned char *pdata, struct cyc_buffer *pcb, int num){
	int i;
	if (pdata == NULL)
		return;
	pcb->header = (++(pcb->header)) % CYCLICAL_BUFFER_SIZE;	
	if ((++(pcb->valid_amount)) > CYCLICAL_BUFFER_SIZE)
	{
			pcb->valid_amount = CYCLICAL_BUFFER_SIZE;
			//DebugWarn("data buffer overflow\n");
	}
	for(i=0;i<POINT_NUM;i++){
		pcb->raw_data[pcb->header].point[i] = ((__int16)((*pdata << 8) + *(pdata + 1))) * DATASCALE / (float)32768;
		//pcb->data[pcb->header].point[i] = 1;
		pdata += 2;
		//printf("point = %f\n", pcb->data[pcb->header].point[i]);
	} 

	if ((Filter_Options & 0x00ff )== FILTER_BUTTERWORTH)
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


inline BOOL Is_connect_ready(void)
{
	int ret;
	ret = select(tcp_socket+1, NULL, &fdwrite, NULL, &con_timeout);
	if (ret <= 0)
	{
		printf("connect timeout!\n");
		return FALSE;
	}
	return TRUE;

}
/* Is_send_ready: using this func to decide whether the socket send buff is ready*/
inline BOOL Is_send_ready(void)
{
	int ret;
	ret = select(tcp_socket+1, NULL, &fdwrite, NULL, &sen_timeout);
	if (ret <= 0)
	{
		printf("send timeout!\n");
		return FALSE;
	}
	return TRUE;
}
/* Is_recv_ready: using this func to decide whether the socket receive buff is ready*/
inline BOOL Is_recv_ready(void)
{
	int ret;
	ret = select(tcp_socket+1, &fdread, NULL, NULL, &rev_timeout);
	if (ret <= 0)
	{
		printf("receive timeout!\n");
		return FALSE;
	}
	return TRUE;
}

/* error_handler: handle the errors occured */
BOOL error_handler(struct protocol_stat *pstat)
{
	//if (pstat->erro == ERROR_ACCESS_DENIED) /* too many entries */
	//return FALSE;
	/* socket data timeout */
	/*if (pstat->error == SOCKET_CONNECT_TIMEOUT)
	{		
	pstat->cmd_stat = PSTAT_SHAKEHAND;
	Sleep(1000);
	return TRUE;
	}*/
	//TODO ddd
	return false;//templly not deal with

	if (pstat->error == SOCKET_DATA_TIMEOUT || pstat->error == SOCKET_CONNECT_TIMEOUT)
	{
		/*pstat->inquire++;
		if (pstat->inquire >= TIMES_TO_INQUIRE) 
		{		
		pstat->cmd_stat = PSTAT_SHAKEHAND;
		pstat->inquire = 0;
		}*/
		printf("try to init socket again!\n");
		pstat->cmd_stat = PSTAT_INIT_SOCKET;
		closesocket(tcp_socket);
		WSACleanup();   //释放套接字资源;
		Sleep(3000);
		return TRUE;
	}
	if (pstat->error == SOCKET_INIT_ERROR)
	{
		return FALSE;
	} 
	return TRUE;
}



