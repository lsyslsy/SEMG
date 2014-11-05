#ifndef _RS485_H_
#define _RS485_H_

#define RS485_MAGIC 'S'

#define RS485_SEND_MODE    	_IO(RS485_MAGIC, 1)	//485 send mode
#define RS485_RECV_MODE     	_IO(RS485_MAGIC, 2)	//485 recv mode
#define RS485_NR 2


#endif