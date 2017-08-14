#pragma once

#include <string>

#include "zmq.h"

class rpc_client {
public:
	rpc_client(int server_nid, int rcvtimeout_ms, int sndtimeout_ms);
	~rpc_client();

	int startup();
	void shutdown();
	bool is_active();

	/*
		sndtimeout_ms / rcvtimeout_ms£º maximum time before a socket operation returns with EAGAIN
			(1) 0: return immediately, with a EAGAIN error if there is no message to receive
			(2) -1: blocking
			(3) other values: wait for a message for that amount of time before returning EAGAIN
	*/
	int do_request(const void *buf, size_t buf_len, char *response, size_t response_len);
private:
	void reset();

public:
	int server_nid;

private:
	void *context;

	void *client; //zmq socket
	int sndtimeout_ms;
	int rcvtimeout_ms;

	volatile bool active;
};