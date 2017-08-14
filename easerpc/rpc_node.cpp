#include "rpc_common.h"
#include "rpc_node.h"

#include "util/cJSON.h"

extern std::unique_ptr<rpc_node> node;

enum RPC_RESCODE {
	RPC_RESCODE_SUCCESS = 0,
	RPC_RESCODE_FAIL,
	RPC_RESCODE_NOT_FOUND,
};

/* ------------------------------- rpc_worker ------------------------------- */
rpc_worker::rpc_worker(int nid, void *ctx) :
nid(nid),
context(ctx),
worker(NULL),
worker_active(false),
worker_stop(false) {

}

rpc_worker::~rpc_worker() {
	this->shutdown();
}

static void *worker_thread_proc(void *data) {
	assert(data);
	rpc_worker *worker = (rpc_worker *)data;

	int rc;
	
	char req_buf[REQ_BUF_MAXSIZE] = { 0 };
	cJSON *req_json = NULL;

	char rep_buf[REP_BUF_MAXSIZE] = { 0 };
	cJSON *rep_json = NULL;

	char *resp_str = NULL;

	while (!worker->worker_stop) {
		//recv
		rc = zmq_recv(worker->worker, req_buf, sizeof(req_buf), 0);
		if (worker->worker_stop) break;
		if (rc == -1) continue;

		PRINT("[%lld] recv request(%s) from client, rc = %d\n", time(NULL), req_buf, rc);

		//process (cJSON is thread-safe?)
		req_json = cJSON_Parse(req_buf);
		short nid = cJSON_GetObjectItem(req_json, "nid")->valueint;
		char *fn = cJSON_GetObjectItem(req_json, "fn")->valuestring;
		char *arg = cJSON_GetObjectItem(req_json, "arg")->valuestring;
		int seq = cJSON_GetObjectItem(req_json, "seq")->valueint;
		const char *ctx = cJSON_GetObjectItem(req_json, "ctx") ?
			cJSON_GetObjectItem(req_json, "ctx")->valuestring : 0;

		rc = node->invoke_function(fn, arg, rep_buf, sizeof(rep_buf));	//should be synchronous

		//build reply
		rep_json = cJSON_CreateObject();
		if (rc == 0) {
			cJSON_AddNumberToObject(rep_json, "code", RPC_RESCODE_SUCCESS);
			cJSON_AddStringToObject(rep_json, "res", *rep_buf == 0 ? "{}" : rep_buf);
		}
		else if (rc == -1)
			cJSON_AddNumberToObject(rep_json, "code", RPC_RESCODE_NOT_FOUND);
		else if (rc == -2)
			cJSON_AddNumberToObject(rep_json, "code", RPC_RESCODE_FAIL);

		cJSON_AddNumberToObject(rep_json, "seq", seq);
		if (ctx) cJSON_AddStringToObject(rep_json, "ctx", ctx);

		resp_str = cJSON_Print(rep_json);

		//send reply back
		rc = zmq_send(worker->worker, resp_str, strlen(resp_str), 0);

		cJSON_Delete(rep_json);
		rep_json = NULL;

		if (worker->worker_stop) break;
		if (rc == -1) continue;

		PRINT("[%lld] send reply(%s) to client, rc = %d\n", time(NULL), rep_buf, rc);

		PRINT("[%lld]------------------------\n\n", time(NULL));
	}

	return NULL;
}

int rpc_worker::startup() {
	if (worker_active) {
		PRINT("rpc_worker::startup() failed: already active.\n");
		return -1;
	}

	int ret = 0;
	int rc;
	char addr[128] = { 0 };

	//start up worker
	worker = zmq_socket(context, ZMQ_REP);

	int rcv_timeout = RPC_WORKER_RCV_TIMEOUT_MS;
	zmq_setsockopt(worker, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout));

	int snd_timeout = RPC_WORKER_SND_TIMEOUT_MS;
	zmq_setsockopt(worker, ZMQ_SNDTIMEO, &snd_timeout, sizeof(snd_timeout));

	sprintf(addr, DEALER_ADDR, nid);
	rc = zmq_connect(worker, addr);
	if (rc) {
		PRINT("rpc_worker::startup() failed: worker connect error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -2;
		goto rpc_worker_startup_fail;
	}

	worker_stop = false;
	rc = pthread_create(&worker_thread, NULL, worker_thread_proc, this);
	if (rc) {
		PRINT("rpc_worker::startup() failed: create worker thread failed %d", rc);
		ret = -3;
		goto rpc_worker_startup_fail;
	}

	worker_active = true;

	return ret;

rpc_worker_startup_fail:

	this->reset();

	return ret;
}

void rpc_worker::shutdown() {
	if (!worker_active) {
		return;
	}

	worker_stop = true;
	pthread_join(worker_thread, NULL);

	this->reset();

}

void rpc_worker::reset() {
	if (worker) {
		zmq_close(worker);
		worker = NULL;
	}

	worker_active = false;
	worker_stop = false;
}

/* ------------------------------- rpc_broker ------------------------------- */
rpc_broker::rpc_broker(int nid) :
nid(nid),
broker_context(NULL),
router(NULL),
router_ip("127.0.0.1"),
router_port(-1),
dealer(NULL),
broker_active(false) {

}

rpc_broker::~rpc_broker() {
	this->shutdown();
}

static void *broker_thread_proc(void *data) {
	assert(data);
	rpc_broker *broker = (rpc_broker *)data;

	zmq_proxy(broker->router, broker->dealer, NULL);	//would block here

	return NULL;
}

int rpc_broker::startup() {
	if (broker_active) {
		PRINT("rpc_broker::startup() failed: already active.\n");
		return -1;
	}

	int ret = 0;
	int rc;
	char addr[128] = { 0 };
	char bind_port_str[255];
	size_t bind_port_strlen = sizeof(bind_port_str);

	//create ctx
	broker_context = zmq_ctx_new();

	//start up router
	router = zmq_socket(broker_context, ZMQ_ROUTER);
	if (!router) {
		PRINT("rpc_broker::startup() failed: router socket create error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -2;
		goto rpc_broker_startup_fail;
	}

	sprintf(addr, "tcp://%s:*", router_ip.c_str());
	//sprintf(addr, "tcp://%s:%d", router_ip.c_str(), 9093);
	rc = zmq_bind(router, addr);
	if (rc) {
		PRINT("rpc_broker::startup() failed: router bind error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -3;
		goto rpc_broker_startup_fail;
	}

	rc = zmq_getsockopt(router, ZMQ_LAST_ENDPOINT, bind_port_str, &bind_port_strlen);
	if (rc) {
		PRINT("rpc_broker::startup() failed: router pick port failed [%s]\n", zmq_strerror(zmq_errno()));
		ret = -4;
		goto rpc_broker_startup_fail;
	}
	sscanf(bind_port_str, "%*[^://]://%*[^:]:%d", &router_port); //sth. like "tcp://127.0.0.1:9999"

	//start up dealer
	dealer = zmq_socket(broker_context, ZMQ_DEALER);
	if (!dealer) {
		PRINT("rpc_broker::startup() failed: dealer socket create error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -5;
		goto rpc_broker_startup_fail;
	}

	sprintf(addr, DEALER_ADDR, nid);
	rc = zmq_bind(dealer, addr);
	if (rc) {
		PRINT("rpc_broker::startup() failed: dealer bind error [%s]\n", zmq_strerror(zmq_errno()));
		ret = -6;
		goto rpc_broker_startup_fail;
	}

	//start up zmq proxy (router & dealer)
	rc = pthread_create(&broker_thread, NULL, broker_thread_proc, this);
	if (rc) {
		PRINT("rpc_broker::startup() failed: create broker thread failed %d", rc);
		ret = -7;
		goto rpc_broker_startup_fail;
	}

	broker_active = true;

	return ret;

rpc_broker_startup_fail:

	this->reset();

	return ret;
}

void rpc_broker::shutdown() {
	if (!broker_active) {
		return;
	}

	//NO NEED to join broker thread ?

	this->reset();
}

void rpc_broker::reset() {
	router_port = -1;
	router_ip.clear();
	if (router) {
		zmq_close(router);
		router = NULL;
	}

	if (dealer) {
		zmq_close(dealer);
		dealer = NULL;
	}

	if (broker_context) {
		zmq_ctx_term(broker_context);
		broker_context = NULL;
	}

	broker_active = false;
}

/* ------------------------------- rpc_func_map ------------------------------- */
rpc_func_map::rpc_func_map() {
	pthread_mutex_init_value(&lock);
	assert(pthread_mutex_init(&lock, NULL) == 0);
}

rpc_func_map::~rpc_func_map() {
	map.clear();

	pthread_mutex_destroy(&lock);
}

int rpc_func_map::register_function(const char *fname, ftype fpointer) {
	if (fname == NULL || *fname == 0)
		return -1;
	if (fpointer == NULL)
		return -2;

	pthread_mutex_lock(&lock);

	auto iter = map.find(fname);
	if (iter != map.end()) {
		pthread_mutex_unlock(&lock);
		return -3;
	}
	
	map.emplace(fname, fpointer);

	pthread_mutex_unlock(&lock);

	return 0;
}

int rpc_func_map::unregister_function(const char *fname) {
	if (fname == NULL || *fname == 0)
		return -1;

	pthread_mutex_lock(&lock);

	auto iter = map.find(fname);
	if (iter == map.end()) {
		pthread_mutex_unlock(&lock);
		return -2;
	}
	map.erase(iter);

	pthread_mutex_unlock(&lock);

	return 0;
}

int rpc_func_map::invoke_function(const char *fname, const char *arg, char *res, size_t res_len) {
	pthread_mutex_lock(&lock);

	auto iter = map.find(fname);
	if (iter == map.end()) {
		pthread_mutex_unlock(&lock);
		return -1;	//not found
	}

	try {
		iter->second(arg, res, res_len);
	}
	catch (...) {
		pthread_mutex_unlock(&lock);
		return -2;	//error occured
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/* ------------------------------- rpc_node ------------------------------- */
rpc_node::rpc_node(int nid) :
nid(nid),
active(false)
{

}

rpc_node::~rpc_node() {
	nid = -1;

	broker.reset();
	worker.reset();
	func_map.reset();
}

int rpc_node::startup() {
	int ret = 0;
	int rc;

	broker.reset(new rpc_broker(nid));
	rc = broker->startup();
	if (rc != 0) {
		ret = -1;
		goto rpc_node_startup_fail;
	}
	PRINT("router %s:%d.\n", broker->router_ip.c_str(), broker->router_port);

	worker.reset(new rpc_worker(nid, broker->broker_context));	//如果worker要和dealer进程内通信，就必须用同一个context
	rc = worker->startup();
	if (rc != 0) {
		ret = -2;
		goto rpc_node_startup_fail;
	}

	func_map.reset(new rpc_func_map());

	active = true;
	PRINT("rpc server started.\n");
	return ret;

rpc_node_startup_fail:
	worker->shutdown();
	broker->shutdown();

	worker.reset();
	broker.reset();
	
	func_map.reset();

	return ret;
}

void rpc_node::shutdown() {
	worker->shutdown();
	broker->shutdown();

	worker.reset();
	broker.reset();

	func_map.reset();

	active = false;
}

bool rpc_node::is_active() {
	return active;
}

int rpc_node::register_function(const char *fname, ftype fpointer) {
	return func_map->register_function(fname, fpointer);
}

int rpc_node::unregister_function(const char *fname) {
	return func_map->unregister_function(fname);
}

int rpc_node::invoke_function(const char *fname, const char *arg, char *res, size_t res_len) {
	return func_map->invoke_function(fname, arg, res, res_len);
}