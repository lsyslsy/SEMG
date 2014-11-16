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
	char dest[18] = "1C:78:39:11:11:11";
	char buf[60000] = {0}; //60KB size
	//allocate a socket
	sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

	//set the connection parameters (who to connect to)
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(0x1001);
	str2ba(dest, &addr.l2_bdaddr);

	if (set_l2cap_mtu(sock, 65000) != 0) {
		perror("set_l2cap_mtu");
		goto out;
	}	
	// if(set_flush_timeout(&remote_addr.l2_bdaddr, 10)) {
	// 		perror("set flush timeout");
	// 		goto failed1;
	// 	}
	//connect to server
	status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (status != 0) {
		goto out;
	}
	for (i = 0; i < 100; i++) {
		sprintf(buf, "you are the best [%d] times", i);
		//send a message
		retval = send(sock, buf, 50000, 0);		
		if (retval < 0) {
			perror("send error");
			goto out;
		}
		printf("send %d\n", i);
		sleep(1);
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