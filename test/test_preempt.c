//
// Created by bochao on 10/25/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "uthread.h"
#define ELAPSED_TIME 10000

int thread_print(void *arg) {
    printf("This is thread %d\n", uthread_self());
    clock_t begin = clock();
    while(1){
        clock_t end = clock();
        if((double)(end-begin)/CLOCKS_PER_SEC > 0.01)
            break;
    }
    printf("Second time: thread %d\n", uthread_self());
    return 0;
}

int main() {
    clock_t begin = clock();
    //Configure a timer that fire an SIGVTALRM 100 times per second
    for (int i = 0; i < 10; ++i) {
        uthread_create(thread_print, NULL);
    }

    printf("This is the main thread!\n");
    while(1){
        clock_t end = clock();
        if((double)(end - begin)/CLOCKS_PER_SEC > 1)
            break;
    }
    printf("All thread should be finished before this time!\n");
    clock_t end = clock();
    printf("Total time is: %lf\n", (double)(end - begin));
}