//
// Created by bochao on 10/25/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include "./libuthread/uthread.h"

int thread_print(void *arg) {
    uthread_join(1, NULL);
    printf("This is thread %d\n", uthread_self());
    return 0;
}

int main() {
    uthread_t thread1 = uthread_create(thread_print, NULL);
    uthread_t thread2 = uthread_create(thread_print, NULL);
    printf("This is the main thread!\n");
    sleep(1);
    printf("Thread %d and %d should be finished!\n", thread1, thread2);
}