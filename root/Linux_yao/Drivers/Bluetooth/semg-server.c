#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "semg.h"

//TODO: change to bluetooth address to specified
int main(int argc, char **argv) {		
	//程序员面试宝典
	struct sockaddr_l2 local_addr, remote_addr;
	//hci_open_route
	char buf[60000] = {0};
	int listensock, clientsock, err = 0, retval = 0;
	unsigned int optlen = sizeof(remote_addr);
	ssize_t count = 0;
	int i;

	listensock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (listensock < 0)
		return listensock;

	if (set_l2cap_mtu(listensock, 65000) != 0) {
		perror("set_l2cap_mtu");
		goto failed;
	}	

	//Bind to local adapter
	memset(&local_addr, 0, sizeof(local_addr));	
	local_addr.l2_family = AF_BLUETOOTH;
	local_addr.l2_bdaddr = *BDADDR_ANY;
	local_addr.l2_psm = htobs(0x1001);
	if (bind(listensock, (struct sockaddr *)&local_addr, sizeof(local_addr)))
		goto failed;
	
	if (listen(listensock, 1) < 0)
		goto failed;
	
	memset(&remote_addr, 0, sizeof(remote_addr));

	for ( ; ; ) {
		clientsock = accept(listensock, (struct sockaddr *)&remote_addr, &optlen);
		if (clientsock < 0) {
			perror("accpet error");
			//syslog(LOG_ERR, "ac")
			goto failed;
		}
		// if(set_flush_timeout(&remote_addr.l2_bdaddr, 10)) {
		// 	perror("set flush timeout");
		// 	goto failed1;
		// }
		i = 0;
		ba2str(&remote_addr.l2_bdaddr, buf);
		fprintf(stderr, "accepted connection from %s\n", buf);
		do {
			count = recv(clientsock, buf, sizeof(buf), 0);
			if(count > 0) {
				printf("received [%d]:%d\n", count, i);
			} else if(count == 0){
				printf("client disconnect\n");
			} else {
				perror("recv error");
			}
			//count = send(clientsock, "yes you are the best!", 20, 0);
			i++;
		} while (count > 0);
	failed1:
		close(clientsock);
	}
	//set MTU(maximum transfer unit)optlen
	//set no retranssimition: set auto flush number to a small value
	//set encry
failed:
	err = errno;
	close(listensock);
	errno = err;

	return -1;
}

