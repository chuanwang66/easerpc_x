#if defined(_DEBUG)

#include <stdio.h>
#include <string.h>

#include "easerpc.h"

#include "util/threading.h"
#include "util/platform.h"

void os_sleep_ms(uint32_t duration);

volatile int global_int = 0;

static void *thread_proc(void *param) {
	int *iptr = (int *)param;
	os_set_thread_name("easerpc_x worker thread");

#if defined(_WIN32)
	os_sleep_ms(2000);
#else
	//TODO
#endif
	global_int = *iptr + 1;

	return NULL;
}

int test_pthread(int i) {
	pthread_t thread;

	if (pthread_create(&thread, NULL, thread_proc, &i) != 0)
		goto fail;
	pthread_join(thread, NULL);

	return global_int;
fail:
	return -1;
}

#endif