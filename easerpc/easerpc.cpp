#include "easerpc.h"
#include "rpc_common.h"
#include "rpc_node.h"
#include "rpc_client.h"

#include <memory>

#include "util/cJSON.h"

std::unique_ptr<rpc_node> node;
static std::unique_ptr<rpc_client> client;

int rpc_initialize(short nid) {

	if (node) {
		PRINT("rpc_initialize() failed: already inited");
		return -1;
	}

	node.reset(new rpc_node(nid));
	if (node->startup() != 0) {
		PRINT("rpc_initialize() failed: startup error");
		return -2;
	}

	return 0;
}

void rpc_destory() {

	if (node) {
		if (node->is_active())
			node->shutdown();
		node.reset();
	}

	if (client) {
		if (client->is_active())
			client->shutdown();
		client.reset();
	}
}

int rpc_register_function(const char *fname, ftype fpointer) {
	if (node) {
		return node->register_function(fname, fpointer);
	}
	return -1000;
}

int rpc_unregister_function(const char *fname) {
	if (node) {
		return node->unregister_function(fname);
	}
	return -1000;
}

volatile long request_seq = 0;

int rpc_request(short nid, const char *fname, const char *args, char *response, size_t response_len, long sndtimeout_ms, long rcvtimeout_ms) {
	//check params
	if (fname == NULL || *fname == 0)
		return -1;
	if (args == NULL || *args == 0)
		args = "{}";
	if (response == NULL || response_len <= 0)
		return -2;

	const char *ctx = NULL;
	int ret = 0;
	int rc;
	cJSON *request_json = NULL;
	char *request_str = NULL;
	cJSON *response_json = NULL;
	char response_str[REP_BUF_MAXSIZE];
	int res_seq = 0;
	char *res_res = NULL;
	char *res_ctx = NULL;

	//build request
	request_json = cJSON_CreateObject();
	if (request_json == NULL) {
		ret = -3;
		goto cleanup_request;
	}
	cJSON_AddNumberToObject(request_json, "nid", nid);
	cJSON_AddStringToObject(request_json, "fn", fname);
	cJSON_AddStringToObject(request_json, "arg", args);
	cJSON_AddNumberToObject(request_json, "seq", os_atomic_inc_long(&request_seq));
	if (ctx) cJSON_AddStringToObject(request_json, "ctx", ctx);

	request_str = cJSON_Print(request_json);
	PRINT("request: \n%s\n", request_str);

	//do request
	if (client && client->server_nid == nid && client->is_active());
	else {
		client.reset();
		client.reset(new rpc_client(nid, rcvtimeout_ms, sndtimeout_ms));
		rc = client->startup();
		if (rc) {
			ret = -4;
			goto cleanup_request;
		}
	}

	if (client->do_request(request_str, strlen(request_str), response_str, sizeof(response_str)) != 0) {
		ret = -5;
		goto cleanup_request;
	}

	//parse response
	response_json = cJSON_Parse(response_str);
	res_seq = cJSON_GetObjectItem(response_json, "seq")->valueint;
	res_res = cJSON_GetObjectItem(response_json, "res")->valuestring;
	res_ctx = cJSON_GetObjectItem(response_json, "ctx") ?
		cJSON_GetObjectItem(response_json, "ctx")->valuestring : 0;

	if (strlen(res_res) + 1 > response_len) {
		memcpy(response, res_res, response_len - 1);
		response[response_len - 1] = 0;
		ret = -6;
		goto cleanup_request;
	}
	memcpy(response, res_res, strlen(res_res));
	response[strlen(res_res)] = 0;

	PRINT("response: \nres_seq=%d\n, res_res=%s\n, res_ctx=%s\n", res_seq, res_res, res_ctx);

	return ret;

cleanup_request:
	cJSON_Delete(request_json);
	request_json = NULL;

	return ret;
}
