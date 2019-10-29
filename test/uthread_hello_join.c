/*
 * Simple hello world test
 *
 * Tests the creation of a single thread and its successful return.
 */

#include <stdio.h>
#include <stdlib.h>

#include "../libuthread/uthread.h"

int hello2(void* arg)
{
	printf("Hello world 2!\n");
	return 0;
}

int hello1(void* arg)
{
	uthread_t tid = uthread_create(hello2, NULL);
	uthread_join(tid, NULL);
	printf("Hello world 1!\n");
	return 0;
}

int main(void)
{
	uthread_t tid;

	tid = uthread_create(hello1, NULL);
	uthread_join(tid, NULL);

	return 0;
}