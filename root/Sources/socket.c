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

extern pthread_cond_t cond_tick;

unsigned int PORT = SOCKET_PORT;
unsigned char shakehand_buffer[SHAKEHAND_SIZE];
unsigned char sendbuff[MAX_TURN_BYTE];//32KB
unsigned int BufLen = MAX_TURN_BYTE;
unsigned int send_size;
unsigned int TimeStamp;
unsigned int send_ready = 0; // 0: not ready, 1: ready
pthread_mutex_t mutex_send;
pthread_cond_t cond_send;

/**
 * Init socket
 */
void socket_init()
{
	int i;
	for (i = 0; i < 128; i++) {
		sendbuff[7 + i * 203] = 0x11;
		sendbuff[8 + i * 203] = i;
	}

	for (i = 0; i < 4; i++) {
		sendbuff[25991 + i * 92] = 0x12;
		sendbuff[25992 + i * 92] = i + 8;
	}
}

/**
 * The main socket function, init socket, listen for conneting
 * if client port is down, func will continue to accept another
 * conneting.
 **/
void FunSocket()
{
	unsigned long tick = 0;
	char cmd;
	int err = -1;
	int length;
	int listensock, connsock;
	int reuse = 1;
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
	// 要加重绑定地址,因为'TCP的实现'在一段时间内不允许重复绑定同一地址
	if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
		perror("sockopt reuse addr error");
		pthread_exit((void *) 1);
	}
	if (bind(listensock, (struct sockaddr *) &serveraddr,
			sizeof(struct sockaddr_in)) < 0) {
		perror("bind socket error");
		pthread_exit((void *) 1);
	}
	if (listen(listensock, 1) < 0) { //接收一个连接请求
		perror("socket listen error");
		pthread_exit((void *) 1);
	}
	while (1)
	{
		connsock = accept(listensock, (struct sockaddr *) NULL, NULL);
		print_socket_info(connsock);
		err = setsockopt(connsock, SOL_SOCKET, SO_SNDBUF, (char *) &BufLen,
				optlen);//bufsize = 65536(32*1024 * 2)
		if (err) {
			perror("setsockopt failed");
			goto out1;
		}
		while (1)
		{
		 	pthread_mutex_lock(&mutex_send);
	 		while (send_ready != 1)
	 			pthread_cond_wait(&cond_send, &mutex_send);
	 		send_ready = 0;
	 		pthread_mutex_unlock(&mutex_send);
			//DebugInfo("socket thread is running!%ld\n", tick++);
			// TODO: check sync 8 branches
			// wait for message from processer

			length = recv(connsock, &cmd, 1, 0);

			if (length <= 0)
			{
				DebugError("client connect down!\n");
				break;
			}
			else
			{
				if((start = times(&tmsstart)) == -1)
					perror("times start error");
					//sleep(2);

				// TODO:可以先select下，看下之前是否发送完成了，但是select返回可写只代表缓冲区有空啊
				// 不一定发送完了，除非知道这种会积累一直到溢出
				length = send_task(connsock, cmd);
				if((end = times(&tmsend)) == -1)
					perror("times end error");
				printf("send used %f ms\n", (end - start)/(double)clktck);
				if (length <= 0)
				{
					DebugError("client connect down!\n");
					break;
				}
			}
		}//while(1)
out1:
		close(connsock);
	}//while(1)
	close(listensock);
}//FunSocket()

/**
 *@brief send_task: send buffer according to the client's cmd
 *@param connsock: client socket
 *@param cmd: client command
 *@return the send data length if succeed, else return SOCKET_ERROR = -1
 **/
int send_task(int connsock, char cmd)
{
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
		sendbuff[5] = 0; // state High
		sendbuff[6] = 0; // state Low
		send_size = 26360;
		sendbuff[1] = (send_size >> 8) & 0x00FF;
		sendbuff[2] = send_size & 0x00FF;
		sendbuff[send_size -1 ] = DATA_END;
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
// void data_packet(unsigned char *pbuf, unsigned int *psize)
// {
// 	int i;
// 	unsigned char * p = pbuf+2;
// 	for (i = 0; i < BRANCH_NUM; i++) //TODO 包长可调
// 	{
// 		if(semg_pool[i][0] == 0x48)
// 			pbuf[0] = 0x48;
// 		if(semg_pool[i][0] == 0xee)
// 			pbuf[1] = pbuf[1] | 0x01 << i;
// 		//temp_size = (data_pool[i][2] << 8) + data_pool[i][3];//每个Branch的长度
// 		//if (temp_size == BRANCH_DATA_SIZE)//不应该在这里验证
// 		//{
// 			///##数据包出错改怎么处理##////
// 			memcpy(p, &semg_pool[i][BRANCH_HEADER_SIZE], SEMG_DATA_SIZE);

// 		//}
// 		*psize += SEMG_DATA_SIZE;//搞毛
// 		p += SEMG_DATA_SIZE;//不管成不成功都要加上去
// 	}
// 	*p = DATA_END;
// 	*psize += 1;
// 	DebugInfo("actual send size:%d\n", *psize);
// }//data_packet()

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