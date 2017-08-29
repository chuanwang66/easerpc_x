#include "rpc_common.h"
#include "rpc_client.h"

#include <string>
#include <time.h>

rpc_client::rpc_client(int server_nid, int rcvtimeout_ms, int sndtimeout_ms) :
server_nid(server_nid),
context(NULL),
client(NULL),
rcvtimeout_ms(rcvtimeout_ms),
sndtimeout_ms(sndtimeout_ms), 
active(false) {

}

rpc_client::~rpc_client() {
	this->shutdown();
}

int rpc_client::startup() {
	if (active) {
		PRINT("rpc_client::startup() failed: already active.\n");
		return -1;
	}

	int ret = 0;
	int rc;
	char addr[128] = { 0 };

	//TODO nid --> zmq router's IP & port
	//sprintf(addr, "tcp://%s:%d", server_ip.c_str(), server_port);
	sprintf(addr, "tcp://%s:%d", "127.0.0.1", 9093);

	//create ctx
	context = zmq_ctx_new();

	//start up client
	client = zmq_socket(context, ZMQ_REQ);
	if (!client) {
		PRINT("rpc_client::startup() failed: client socket create error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -2;
		goto rpc_client_startup_fail;
	}

	zmq_setsockopt(client, ZMQ_RCVTIMEO, &rcvtimeout_ms, sizeof(rcvtimeout_ms));
	zmq_setsockopt(client, ZMQ_SNDTIMEO, &sndtimeout_ms, sizeof(sndtimeout_ms));

	rc = zmq_connect(client, addr);
	if (rc) {
		PRINT("rpc_client::startup() failed: client socket connect failed [%s]\n", zmq_strerror(zmq_errno()));
		ret = -3;
		goto rpc_client_startup_fail;
	}

	active = true;

	return ret;

rpc_client_startup_fail:
	this->reset();

	return ret;
}

void rpc_client::shutdown() {
	if (!active) {
		return;
	}

	this->reset();
}

bool rpc_client::is_active() {
	return active;
}

void rpc_client::reset() {
	server_nid = -1;

	if (client) {
		zmq_close(client);
		client = NULL;
	}
	sndtimeout_ms = -1;
	rcvtimeout_ms = -1;

	if (context) {
		zmq_ctx_term(context);
		context = NULL;
	}

	active = false;
}

int rpc_client::do_request(const void *buf, size_t buf_len, char *response, size_t response_len) {
	if (buf == NULL || buf_len <= 0)
		return -1;
	if (response == NULL || response_len <= 0)
		return -2;

	if (zmq_send(client, buf, buf_len, 0) == -1) {
		PRINT("[%lld] rpc_client::do_request() send failed.\n", time(NULL));
		return -3;
	}

	memset(response, 0, response_len);
	if (zmq_recv(client, response, response_len, 0) == -1) {
		PRINT("[%lld] rpc_client::do_request() recv failed.\n", time(NULL));
		return -4;
	}
	
	return 0;
}
