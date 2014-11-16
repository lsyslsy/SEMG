#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int main(int argc, char **argv)
{
	struct sockaddr_rc addr = {0};
	int sock, status;
	char dest[18] = "1C:78:39:11:11:11";

	//allocate a socket
	sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	//set the connection parameters (who to connect to)
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = 1;
	str2ba(dest, &addr.rc_bdaddr);

	//connect to server
	status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	//send a message
	if ( status == 0) {
		status = send(sock, "hello!", 6, 0);
	}

	if (status < 0) {
		perror("uh oh");
	}

	close(sock);
	return 0;
	
}