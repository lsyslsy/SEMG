/*
 * socket.c: The socket file for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/times.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../Headers/main.h"
#include "../Headers/socket.h"
#include "../Headers/led.h"
#include "../Headers/semg_debug.h"

unsigned int PORT = SOCKET_PORT;
unsigned char sendbuff[MAX_TURN_BYTE];//32KB
unsigned int BufLen = MAX_TURN_BYTE;
unsigned int send_size;
unsigned int TimeStamp;
extern struct root root_dev;
extern pthread_mutex_t mutex_buff;
extern pthread_cond_t cond_tick;
extern unsigned char data_pool[BRANCH_NUM][BRANCH_BUF_SIZE];
extern char used_branch[BRANCH_USED_NUM]; 
/**
 * The main socket function, init socket, listen for conneting
 * if client port is down, func will continue to accept another
 * conneting.
 **/
void FunSocket()
{
	//pid_t fd;
	pthread_cleanup_push(pthread_mutex_unlock, (void *)&mutex_buff);
	unsigned long tick = 0;
	char cmd;
	int err = -1;
	int length;
	int listensock, connsock;
	struct sockaddr_in serveraddr;
	clock_t start, end;
	struct tms tmsstart, tmsend;
	socklen_t optlen = sizeof(BufLen);
	unsigned int clktck;
	
	if((clktck = sysconf(_SC_CLK_TCK)) < 0)
	perror("sysconf error");
	if ((listensock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("create socket error");
		pthread_exit((void *) 1);
	}
	bzero(&serveraddr, sizeof(struct sockaddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//监听所有IP地址
	serveraddr.sin_port = htons(PORT);
	//可能要加重绑定地址
	bind(listensock, (struct sockaddr *) &serveraddr,
			sizeof(struct sockaddr_in));
	listen(listensock, 1);//接收一个连接请求
	while (1)
	{
		connsock = accept(listensock, (struct sockaddr *) NULL, NULL);
		print_socket_info(connsock);
		err = setsockopt(connsock, SOL_SOCKET, SO_SNDBUF, (char *) &BufLen,
				optlen);//bufsize = 65536(32*1024 * 2)
		while (1)
		{
			pthread_mutex_lock(&mutex_buff);
			pthread_cond_wait(&cond_tick, &mutex_buff);
			//DebugInfo("socket thread is running!%ld\n", tick++);
			length = recv(connsock, &cmd, 1, 0);

			if (length <= 0)
			{
				DebugError("client connect down!\n");
				pthread_mutex_unlock(&mutex_buff);
				break;
			}
			else
			{
				if((start = times(&tmsstart)) == -1)
					perror("times start error");
					//sleep(2);
					
				length = send_task(connsock, cmd);
				if((end = times(&tmsend)) == -1)
					perror("times end error");
				printf("send used %f ms\n", (end - start)/(double)clktck);
				if (length <= 0)
				{
					DebugError("client connect down!\n");
					pthread_mutex_unlock(&mutex_buff);
					break;
				}
				pthread_mutex_unlock(&mutex_buff);
			}
		}//while(1)
		close(connsock);
	}//while(1)
	close(listensock);
	pthread_cleanup_pop(0);
}//FunSocket()

/**
 *@brief send_task: send buffer according to the client's cmd
 *@param connsock: client socket
 *@param cmd: client command
 *@return the send data length if succeed, else return SOCKET_ERROR = -1
 **/
int send_task(int connsock, char cmd)
{
	unsigned char *pbuf;
	int ret = -1;
	switch (cmd)
	{
	case HAND_SHAKE:
		sendbuff[0] = RE_HAND_SHAKE;
		sendbuff[1] = (root_dev.version >> 8) & 0x00FF;
		sendbuff[2] = root_dev.version & 0x00FF;
		sendbuff[3] = root_dev.channel_num;
		sendbuff[4] = root_dev.AD_rate;
		ret = send(connsock, sendbuff, SHAKEHAND_SIZE, 0);
		break;
	case DATA_REQUEST:
		TimeStamp++;
		sendbuff[0] = RE_DATA_REQUEST;
		sendbuff[3] = (TimeStamp >> 8) & 0x00FF;
		sendbuff[4] = TimeStamp & 0x00FF;
		sendbuff[5] = 0;
		sendbuff[6] = 0;
		send_size = 7;
		pbuf = &sendbuff[0];
		pbuf += 5;
		//是一次性打包好  还是每个通道线程自己打包的好 这样怎能判断所有通道打包好了
		data_packet(pbuf, &send_size);
		//	printf("[3]:%x,[4]:%x,[5]:%x,[6]:%x,[7]:%x,[8]:%x,[9]:%x,[10]:%x\n",
		//			sendbuff[3], sendbuff[4], sendbuff[5], sendbuff[6],
		//			sendbuff[7], sendbuff[8], sendbuff[9], sendbuff[10]);
		sendbuff[1] = (send_size >> 8) & 0x00FF;
		sendbuff[2] = send_size & 0x00FF;
		ret = send(connsock, sendbuff, send_size, 0);
		break;
		//case CONFIRM_ACK:
		//break;
	default:
		DebugError("cmd from pc is wrong!\n");
		break;
	}//switch(*pcmd)
	return ret;
}//send_task()

/**
 *data_packet: packet the branch buffers into macro package
 *@param pbuf: the macro package buffer point
 *@param psize: the macro package length buffer point
 **/
void data_packet(unsigned char *pbuf, unsigned int *psize)
{
	int i, j;
	unsigned int temp_size;
	unsigned char * p = pbuf+2;
	for (j = 0; j < BRANCH_USED_NUM; j++)//包长可调
	{
		i = used_branch[j];
		if(data_pool[i][0] == 0x48)
			pbuf[0] = 0x48;
		if(data_pool[i][0] == 0xee)
			pbuf[1] = pbuf[1] | 0x01 << i;
		//temp_size = (data_pool[i][2] << 8) + data_pool[i][3];//每个Branch的长度
		//if (temp_size == BRANCH_DATA_SIZE)//不应该在这里验证
		//{
			///##数据包出错改怎么处理##////
			memcpy(p, &data_pool[i][BRANCH_Header_SIZE], BRANCH_DATA_SIZE);

		//}
		*psize += BRANCH_DATA_SIZE;//搞毛
		p += BRANCH_DATA_SIZE;//不管成不成功都要加上去
	}
	*p = DATA_END;
	*psize += 1;
	DebugInfo("actual send size:%d\n", *psize);
}//data_packet()

void print_socket_info(int connsock)
{
	struct sockaddr_in serveraddr, clientaddr;
	char abuf[INET_ADDRSTRLEN];//addr buffer
	int server_len = sizeof(serveraddr);
	int client_len = sizeof(clientaddr);
	//server
	getsockname(connsock, (struct sockaddr *)&serveraddr, &server_len);
	inet_ntop(AF_INET, &serveraddr.sin_addr, abuf, sizeof(abuf));
	printf("host at %s:%d\n", abuf, ntohs(serveraddr.sin_port));
	//client
	getpeername(connsock, (struct sockaddr *)&clientaddr, &client_len);
	inet_ntop(AF_INET, &clientaddr.sin_addr, abuf, sizeof(abuf));
	printf("remote at %s:%d\n", abuf, ntohs(clientaddr.sin_port));
}
