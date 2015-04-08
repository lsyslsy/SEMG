#include <stdio.h>
#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>


char dest[18] = "1C:78:39:11:11:11";
char src[18] = "1C:78:39:22:22:22";

int main(int argc, char const *argv[])
{
	uint32_t svc_uuid_int[] = {0, 0, 0, 0xABCD};
	int status;
	bdaddr_t target, source;
	uuid_t svc_uuid;
	sdp_list_t *response_list, *search_list, *attrid_list;
	sdp_session_t *session = 0;
	uint32_t range = 0x0000ffff;
	uint8_t port = 0;


	//connect to the SDP server running on the remote machine
	str2ba(dest, &target);
	str2ba(src, &source);

	session = sdp_connect(&source, &target, 0);
	if (session == NULL) {
		perror("sdp_connect");
		exit(1);
	}

	sdp_uuid128_create(&svc_uuid, &svc_uuid_int);
	search_list = sdp_list_append(0, &svc_uuid);
	attrid_list = sdp_list_append(0, &range);

	//get a list of service records that have UUID 0xabcd
	response_list = NULL;
	status = sdp_service_search_attr_req(session, search_list, SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
	if (status == 0) {
		sdp_list_t *proto_list = NULL;
		sdp_list_t *r = response_list;

		//go through each of the service records
		for (; r; r = r->next) {
			sdp_record_t *rec = (sdp_record_t *)r->data;

			//get a list of the protocol sequences
			if (sdp_get_access_protos(rec, &proto_list) == 0) {
				// get the RFCOMM port number
				port = sdp_get_proto_port(proto_list, RFCOMM_UUID);
				sdp_list_free(proto_list, 0);
			}
			sdp_record_free(rec);
		}
	}
	sdp_list_free(response_list, 0);
	sdp_list_free(search_list, 0);
	sdp_list_free(attrid_list, 0);
	sdp_close(session);

	if (port != 0) {
		printf("found service running on RFCOMM Port %d\n", port);
	}

	return 0;
}