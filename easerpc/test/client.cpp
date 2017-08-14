#if defined(_DEBUG)

#include <stdio.h>
#include <time.h>

#include "zmq.h"

#include "easerpc.h"
#include "util/platform.h"

void os_sleep_ms(uint32_t duration);

int test_zmq_client(const char *ip, int port, int index) {
	int major, minor, patch;
	zmq_version(&major, &minor, &patch);
	printf("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
	printf("===========================================\n\n");

	char addr[128] = { 0 };
	sprintf(addr, "tcp://%s:%d", ip, port);

	void* context = zmq_ctx_new();
	void* request = zmq_socket(context, ZMQ_REQ);
	int rec = zmq_connect(request, addr);
	if (rec) {
		printf("zmq_connect %s error:%d\n", addr, rec);
		zmq_close(request);
		zmq_ctx_term(context);
		return -2;
	}

	printf("client zmq_connect %s done!\n", addr);

	char buf[128] = { 0 };

	while (1) {
		//sprintf(buf, "index=%d&cmd=hello&time=%lld", index, time(NULL));
		sprintf(buf, "{'nid':3, 'fn':'add', 'arg':'{}', 'seq':1}");
		rec = zmq_send(request, buf, strlen(buf), 0);
		printf("[%lld] send request(%s) to server, rec = %d, request.len = %d\n", time(NULL), buf, rec, strlen(buf));

		rec = zmq_recv(request, buf, sizeof(buf), 0);
		printf("[%lld] recv reply(%s) from server, rec = %d, reply.len = %d\n", time(NULL), buf, rec, strlen(buf));

		printf("[%lld] --------------------------\n\n", time(NULL));
		os_sleep_ms(1000);
	}

	zmq_close(request);
	zmq_ctx_term(context);

	printf("good bye and good luck!\n");
	return 0;
}

#endif