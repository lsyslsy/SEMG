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
#include <sys/time.h>
#include <unistd.h>
#include "main.h"
#include "socket.h"
#include "led.h"

unsigned int PORT = SOCKET_PORT ;
unsigned char sendbuff[MAX_TURN_BYTE];
unsigned int BufLen = MAX_TURN_BYTE;
unsigned int send_size;
unsigned int TimeStamp;
extern struct root root_dev;
extern pthread_mutex_t mutex_buff;
extern pthread_cond_t cond_tick;
extern unsigned char data_pool[BRANCH_NUM][2000];

/**
  *FunSocket_hander: The mutex thread-safe function
  *@arg: NULL parameter point
**/
void FunSocket_hander(void *arg)
{
     free(arg);
     (void)pthread_mutex_unlock(&mutex_buff);
}
/**
  *FunSocket: The main socket function, init socket, listen for conneting
              if client port is down, func will continue to accept another
              conneting.
**/
void FunSocket()
{
 	//pid_t fd;
        pthread_cleanup_push(FunSocket_hander, &mutex_buff);
        unsigned long tick = 0;
        char cmd;
	int err=-1;
	int length;  
        int listensock,connsock;
	struct sockaddr_in serveraddr;
        socklen_t optlen = sizeof(BufLen); 
	listensock = socket(AF_INET,SOCK_STREAM,0); 
	bzero(&serveraddr,sizeof(struct sockaddr)); 
	serveraddr.sin_family = AF_INET; 
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
  	serveraddr.sin_port = htons(PORT); 
 
	bind(listensock,(struct sockaddr *)&serveraddr,sizeof(struct sockaddr_in));
	listen(listensock,1); 
	while(1)
	{
          connsock = accept(listensock,(struct sockaddr *)NULL, NULL);
       	  err = setsockopt(connsock,SOL_SOCKET,SO_SNDBUF, (char *)&BufLen, optlen);//size = 65536(32*1024 * 2)
	   while(1){
                pthread_mutex_lock(&mutex_buff);
                pthread_cond_wait(&cond_tick,&mutex_buff);
                printf("socket thread is running!%ld\n",tick++);
                length = recv(connsock,&cmd,1,0);
                if (length <= 0){
                     printf("client connect down!\n");
                     pthread_mutex_unlock(&mutex_buff);
 	             break;
	        }
                else{
 		     length = send_task(connsock,cmd);
                     if (length <= 0){
                     	  printf("client connect down!\n");
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
  *send_task: send buffer according to the client's cmd
  *@connsock: server accept port
  *@cmd: client command
  *Return the send data length if succeed, else return SOCKET_ERROR = -1
**/
int send_task(int connsock, char cmd)
{
   unsigned char *pbuf;
   int ret = -1;
   switch(cmd)
	{
	case HAND_SHAKE:
             sendbuff[0] = RE_HAND_SHAKE;
	     sendbuff[1] = (root_dev.version >> 8) & 0x00FF;
	     sendbuff[2] = root_dev.version & 0x00FF;
             sendbuff[3] = root_dev.channel_num;
	     sendbuff[4] = root_dev.AD_rate;
	     ret = send(connsock,sendbuff,SHAKEHAND_SIZE,0);
		break;
	case DATA_REQUEST:
             TimeStamp++;
	     sendbuff[0] = RE_DATA_REQUEST;
             sendbuff[3] = (TimeStamp >> 8) & 0x00FF;
             sendbuff[4] = TimeStamp & 0x00FF;
	     send_size = 5;
             pbuf = &sendbuff[0];
             pbuf +=5;
             data_packet(pbuf,&send_size);
             printf("[3]:%x,[4]:%x,[5]:%x,[6]:%x,[7]:%x,[8]:%x,[9]:%x,[10]:%x\n",sendbuff[3],sendbuff[4],sendbuff[5],sendbuff[6],sendbuff[7],sendbuff[8],sendbuff[9],sendbuff[10]); 
	     sendbuff[1] =  (send_size >> 8) & 0x00FF;
             sendbuff[2] =  send_size & 0x00FF;     
 	     ret = send(connsock,sendbuff,send_size,0);
		break;
	//case CONFIRM_ACK:
       	        //break;
	default:
              printf("cmd is wrong!\n");
	        break;
	}//switch(*pcmd)
    return ret;
}//send_task()

/**
  *data_packet: packet the branch buffers into macro package
  *@pbuf: the macro package buffer point
  *@psize: the macro package length buffer point
**/
void data_packet(unsigned char *pbuf, unsigned int *psize) 
{
    int i,j;
    unsigned int temp_size;
    //unsigned char	 *pdata;
    for(i=0;i<BRANCH_NUM;i++)
   {
       //pdata = data_pool[i];
       temp_size = (data_pool[i][2] << 8) + data_pool[i][3];
       if (temp_size == CYCLE_SIZE){
       	   for (j=0;j<CYCLE_SIZE;j++){
       		*pbuf = data_pool[i][4 + j];
       		 pbuf++;
           }
           *psize+=CYCLE_SIZE;
       }     
   }
   *pbuf = END_TURN;
   *psize +=1;
    printf("size:%d\n",*psize);
}//data_packet()


















