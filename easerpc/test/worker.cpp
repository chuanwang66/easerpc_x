#if defined(_DEBUG)

#include <stdio.h>
#include <time.h>

#include "zmq.h"

#include "easerpc.h"
#include "util/platform.h"

void os_sleep_ms(uint32_t duration);

int test_zmq_worker(const char *ip, int port, int index) {
	int major, minor, patch;
	zmq_version(&major, &minor, &patch);
	printf("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
	printf("===========================================\n\n");

	char addr[128] = { 0 };
	sprintf(addr, "tcp://%s:%d", ip, port);

	void* context = zmq_ctx_new();
	void* worker = zmq_socket(context, ZMQ_REP);
	int rec = zmq_connect(worker, addr);
	if (rec) {
		printf("zmq_connect %s error:%d\n", addr, rec);
		zmq_close(worker);
		zmq_ctx_term(context);
		return -2;
	}

	printf("worker zmq_connect %s done!\n", addr);

	char buf[128] = { 0 };

	while (1) {
		rec = zmq_recv(worker, buf, sizeof(buf), 0);
		printf("[%lld] recv request(%s) from client, rec = %d, request.len = %d\n", time(NULL), buf, rec, strlen(buf));
		
		sprintf(buf, "worker=%d&result=world&time=%lld", index, time(NULL));
		rec = zmq_send(worker, buf, strlen(buf), 0);
		printf("[%lld] send reply(%s) to client, rec = %d, reply.len = %d\n", time(NULL), buf, rec, strlen(buf));

		printf("[%lld]------------------------\n\n", time(NULL));
	}

	zmq_close(worker);
	zmq_ctx_term(context);

	printf("good bye and good luck!\n");
	return 0;
}

int test_zmq_server(const char *ip, int port) {
	char addr[128] = { 0 };
	sprintf(addr, "tcp://%s:%d", ip, port);

	//  Socket to talk to clients
	void *context = zmq_ctx_new();
	void *responder = zmq_socket(context, ZMQ_REP);
	int rc = zmq_bind(responder, addr);
	if (rc) {
		printf("[%lld] server bind error : %s\n", time(NULL), zmq_strerror(zmq_errno()));
		zmq_close(responder);
		zmq_ctx_term(context);
		return -1;
	}

	while (1) {
		char buffer[10];
		zmq_recv(responder, buffer, 10, 0);
		printf("Received Hello\n");

		//DO sth.
		os_sleep_ms(1000);

		zmq_send(responder, "World", 5, 0);
	}
	return 0;
}

#endif