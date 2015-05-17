/*
 * socket.h: The header file of socket.c for root board of the sEMG project
 *
 *	All right reserved.
 *
 *	Created: HY@2013.5.10
 */
#ifndef _SOCKET_H
#define _SOCKET_H

#define MAX_TURN_BYTE	   32*1024
#define SOCKET_PORT        5000
//#define POINT_NUM          100
//#define CYCLICAL_BUFFER_SIZE	 10

#define SHAKEHAND_SIZE     5

enum PC_CMD {
	HAND_SHAKE    = 'H',
	//ELECTRIC_INFO = 'E',
	CONFIRM_ACK   = 'A',
    DATA_REQUEST = 'D',
	//LOWER_UPT_RAT = 'L',
	//UPPER_UPT_RAT = 'U',
};

enum ROOT_REPLY_PACKET_CLASS {
	RE_HAND_SHAKE    = 'h',
	//RE_ELECTRIC_INFO = 'e',
	RE_DATA_REQUEST	 = 'd'
};

enum ROOT_CMD {
        START = 0xa1,
        DATA  = 0xa2
};

enum BRANCH_REPLY_PACKET_CLASS {
	RE_START    = 0xa3,
	//RE_ELECTRIC_INFO = 'e',
	RE_DATA	    = 0xa4
};

enum DATA_PACKET_CLASS {
	ALL_DATA       = 0x11,
	//ELECTRIC_DATA      = 0x11,
	DATA_END        = 0xED
};
void FunSocket_hander(void *arg);
void FunSocket();
int send_task(int connsock, char cmd);
void data_packet(unsigned char *pbuf, unsigned int *psize);
void print_socket_info(int socketfd);

#endif