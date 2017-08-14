#pragma once

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////// rpc server error code //////////////////////////////
enum RPC_RESCODE {
	RPC_RESCODE_SUCCESS = 0,
	RPC_RESCODE_FAIL,
	RPC_RESCODE_NOT_FOUND,
};

//Rpc remote function prototype
typedef void(*ftype)(const char *command, char *res, size_t res_maxsize);

//Initialize a rpc stub-pair (a rpc server stub & a rpc client stub), sharing the same node id (nid)
//	return: 0 on success
//  NOTE: call rpc_server_initialize() once per process
EXPORT int rpc_initialize(short nid);

//Register rpc remote function (acting as a rpc server stub)
//	return: 0 on success
EXPORT int rpc_register_function(const char *fname, ftype fpointer);

//Unregister rpc remote function (acting as a rpc server stub)
//	return: 0 on success
EXPORT int rpc_unregister_function(const char *fname);

//Destroy the rpc stub-pair
EXPORT void rpc_destory();

//rpc client stub --> rpc server stub --> rpc client stub (react via mapping file)
//  milliseconds: request timeout in millisecond, -1 for blocking, 0 for non-blocking
//	return: 0 on rpc success, otherwise failure
//			(1) if 0 is returned, the rpc reaction itself succeeds, and the response returned must be a json like "{code=%d, ...}"
//				code == RPC_RESCODE_SUCCESS		==> rpc function works well
//				code == RPC_RESCODE_FAIL		==> rpc function goes wrong
//				code == RPC_RESCODE_NOT_FOUND	==> rpc function not found
//			(2) if a negative is returned, the rpc reaction itself goes wrong, in which case you should ignore the response
//
//NOTE: This method is THREAD-UNSAFE (for I'm not sure about the thread-safety of cJSON)
//TODO: Asynchronous callback is not supported yet.
EXPORT int rpc_request(short nid, const char *fname, const char *args, char *response, size_t response_len, long sndtimeout_ms, long rcvtimeout_ms);

#if defined(_DEBUG)
EXPORT int test_pthread(int);

EXPORT int test_zmq_client(const char *ip, int port, int index);
EXPORT int test_zmq_broker(const char *router_ip, int router_port, const char *dealer_ip, int dealer_port);
EXPORT int test_zmq_worker(const char *ip, int port, int index);

EXPORT int test_zmq_server(const char *ip, int port);
#endif

#ifdef __cplusplus
}
#endif