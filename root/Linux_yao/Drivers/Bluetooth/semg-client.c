#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "semg.h"

int main(int argc, char **argv)
{
	struct sockaddr_l2 addr = {0};
	int sock, status, i, retval;
	char dest[18] = "1C:78:39:11:11:11";//"00:1A:7D:DA:71:13";//
	char buf[60000] = {0}; //60KB size
		struct timespec 		tpstart, tpend;
	clock_t 		start, end;
	//allocate a socket
	sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

	if (sock < 0) {
		perror("socket");
		goto out;
	}

	//set the connection parameters (who to connect to)
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(0x1001);
	str2ba(dest, &addr.l2_bdaddr);

	if (set_l2cap_mtu(sock, 65000) != 0) {
		perror("set_l2cap_mtu");
		goto out;
	}	
	// if(set_flush_timeout(&remote_addr.l2_bdaddr, 1)) {
	// 		perror("set flush timeout");
	// 		goto failed1;
	// 	}
	//connect to server
	status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (status != 0) {
		perror("connect");
		goto out;
	}
	for (i = 0; i < 1; i++) {
		sprintf(buf, "you are the best [%d] times", i);
		if ((start = clock_gettime(CLOCK_REALTIME, &tpstart)) == -1) {
			printf("times error");
			return -1;
		}
		//send a message
		retval = send(sock, buf, 28000, 0);	
		if (retval < 0) {
			perror("send error");
			goto out;
		}
		if ((end = clock_gettime(CLOCK_REALTIME, &tpend)) == -1) {
			printf("times error");
			return -1;
		}
	
		printf("send %d\t", i);
		pri_times(end - start, &tpstart, &tpend);
		sleep(1);//proper delay may be necessary
		/*retval = recv(sock, buf, sizeof(buf), 0);
		if (retval < 0) {
			perror("recv error");
			goto out;
		}
		if (retval == 0) goto out;
		if (retval > 0) {
			printf("received ack [%s] %d\n", buf, i);
		}
		sleep(10);*/

	}
out:
	close(sock);
	return 0;
	
}