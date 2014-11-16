#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

//actual wait time = timeout *0.625ms
static int set_flush_timeout(bdaddr_t *ba, int timeout) {
	int err = 0, dd;
	struct hci_conn_info_req *cr = 0;
	struct hci_request rq = {0};


	struct {
		uint16_t handle;
		uint16_t flush_timeout;
	}  cmd_param ;

	struct {
		uint8_t status;
		uint16_t handle;
	}  cmd_response ;

	//find the connection handle to the specified bluetooth device
	cr = (struct hci_conn_info_req *)malloc(sizeof(struct hci_conn_info_req) + 
			sizeof(struct hci_conn_info));
	bacpy(&cr->bdaddr, ba);
	cr->type = ACL_LINK;
	dd = hci_open_dev(hci_get_route(&cr->bdaddr));
	if (dd < 0) {
		err = dd;
		goto out;
	}
	err = ioctl(dd, HCIGETCONNINFO, (unsigned long)cr);
	if (err) goto out;

	//build a command packet to send to the bluetooth microcontroller
	cmd_param.handle = cr->conn_info->handle;
	cmd_param.flush_timeout = htobs(timeout);
	rq.ogf = OGF_HOST_CTL;
	rq.ocf = 0x28;
	rq.cparam = &cmd_param;
	rq.clen = sizeof(cmd_param);
	rq.rparam = &cmd_response;
	rq.rlen = sizeof(cmd_response);
	rq.event = EVT_CMD_COMPLETE;

	//send the command and wait for the response
	err = hci_send_req(dd, &rq, 0);
	if(err) goto out;

	if(cmd_response.status) {
		err = -1;
		errno = bt_error(cmd_response.status);
	}

out:
	free(cr);
	if (dd >= 0) close(dd);
	return err;
}

//mtu need <= 65535
static int set_l2cap_mtu( int sock, uint16_t mtu ) {
	struct l2cap_options opts;
    int optlen = sizeof(opts), err;
    err = getsockopt( sock, SOL_L2CAP, L2CAP_OPTIONS, &opts, (socklen_t *)&optlen );
    if( !err ) {
        opts.omtu = opts.imtu = mtu;
        err = setsockopt( sock, SOL_L2CAP, L2CAP_OPTIONS, &opts, (socklen_t)optlen );
    }
    return err;
};

