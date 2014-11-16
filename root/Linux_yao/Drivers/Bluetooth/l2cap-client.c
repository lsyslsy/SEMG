#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char **argv)
{
	struct sockaddr_l2 addr = {0};
	int sock, status, i, retval;
	char dest[18] = "1C:78:39:11:11:11";
	char buf[10240] = {0};
	//allocate a socket
	sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

	//set the connection parameters (who to connect to)
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(0x1001);
	str2ba(dest, &addr.l2_bdaddr);

	//connect to server
	status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (status != 0) {
		goto out;
	}

	sprintf(buf, "you are the best, hello world", i);
	//send a message
	retval = send(sock, buf, 15, 0);		
	if (retval < 0) {
		perror("send error");
		goto out;
	}
	
	
out:
	close(sock);
	return 0;
	
}