#if defined(_WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cJSON.h"
#include "easerpc.h"

void add(const char *arg, char *res, size_t res_maxsize) {
	int sum;
	cJSON *res_json = cJSON_CreateObject();
	cJSON *param1 = NULL;
	cJSON *param2 = NULL;

	//1. parse
	cJSON *arg_json = cJSON_Parse(arg);
	if (arg_json) {
		param1 = cJSON_GetObjectItem(arg_json, "param1");
		param2 = cJSON_GetObjectItem(arg_json, "param2");
	}

	//2. process
	if (param1 && param2) {
		sum = param1->valueint + param2->valueint;
		cJSON_AddNumberToObject(res_json, "sum", sum);
		cJSON_AddNumberToObject(res_json, "res", 0);
		cJSON_AddStringToObject(res_json, "msg", "");
	}
	else {
		cJSON_AddNumberToObject(res_json, "res", -1);
		cJSON_AddStringToObject(res_json, "msg", "invalid params");
	}

	//3. response
	char *res_str = cJSON_Print(res_json);

	strncpy(res, res_str, res_maxsize);
	res[res_maxsize - 1] = 0;
	
	cJSON_Delete(arg_json);
	cJSON_Delete(res_json);
}

void dummy(const char *command, char *res, size_t res_maxsize) {
	strncpy(res, "{}", res_maxsize);	//overflow safe
	res[res_maxsize - 1] = 0;	//null terminated
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("usage: %s sid\n", argv[0]);
		exit(1);
	}

	int server_id = 0;
	if (sscanf(argv[1], "%d", &server_id) != 1) {
		printf("usage: %s sid [sid should be an integer!]\n", argv[0]);
		exit(2);
	}

	printf("rpc stub id: %d, 's'(start stub), 't'(stop stub) ...\n", server_id);

	int ret;
	while (true) {
		while (getchar() != 's');
		ret = rpc_initialize(server_id);
		if (ret == 0) {
			rpc_register_function("add", add);
			rpc_register_function("dummy", dummy);
			rpc_unregister_function("dummy");

			while (getchar() != 't');

			rpc_destory();
			//rpc_destory();
		}
		else {
			printf("rpc_server_initialize() failed: %d\n", ret);
		}
	}

	return 0;
}
