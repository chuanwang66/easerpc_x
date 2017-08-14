#pragma once

#include <assert.h>
#include <memory>
#include <unordered_map>

#include "rpc_common.h"
#include "util/threading.h"

#include "zmq.h"

/* ------------------------------- rpc_worker ------------------------------- */
#define RPC_WORKER_RCV_TIMEOUT_MS 500
#define RPC_WORKER_SND_TIMEOUT_MS 500

class rpc_worker {
public:
	rpc_worker(int nid, void *ctx);
	~rpc_worker();

	int startup();
	void shutdown();

private:
	void reset();

public:
	int nid;

	void *context; //refers to broker's context

	void *worker; //zmq socket

	pthread_t worker_thread;
	volatile bool worker_active;
	volatile bool worker_stop; //signal to stop worker thread
};

/* ------------------------------- rpc_broker ------------------------------- */
class rpc_broker {
public:
	rpc_broker(int nid);

	~rpc_broker();

	int startup();
	void shutdown();

private:
	void reset();

public:
	int nid;

	void *broker_context;

	void *router; //zmq socket
	std::string router_ip;
	int router_port;

	void *dealer; //zmq socket

	pthread_t broker_thread;
	volatile bool broker_active;
};

/* ------------------------------- rpc_func_map ------------------------------- */
typedef void(*ftype)(const char *command, char *res, size_t res_maxsize);
typedef std::unordered_map<std::string, ftype> func_map;

class rpc_func_map {
public:
	rpc_func_map();
	~rpc_func_map();

	int register_function(const char *fname, ftype fpointer);
	int unregister_function(const char *fname);
	int invoke_function(const char *fname, const char *arg, char *res, size_t res_len);
public:
	func_map map;
	pthread_mutex_t lock;
};

/* ------------------------------- rpc_node ------------------------------- */
class rpc_node {
public:
	rpc_node(int nid);
	~rpc_node();

	int startup();
	void shutdown();

	bool is_active();

	int register_function(const char *fname, ftype fpointer);
	int unregister_function(const char *fname);
	int invoke_function(const char *fname, const char *arg, char *res, size_t res_len);
private:
	bool active;
	int nid;
	std::unique_ptr<rpc_broker> broker;
	std::unique_ptr<rpc_worker> worker;
	std::unique_ptr<rpc_func_map> func_map;
};