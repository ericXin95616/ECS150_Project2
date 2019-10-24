//
// Created by bochao on 10/22/19.
//
#include "./libuthread/uthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

int hello(void *arg){
    uthread_t tid = uthread_self();
    int n = (int)(long)arg;
    if(tid >= n){
        printf("Hello! I am thread %d. I am the last thread!\n", tid);
        return tid;
    }

    printf("Hello! I am thread %d. I am waiting for thread %d\n", tid, tid + 1);
    int retval;
    uthread_join(tid+1, &retval);
    printf("Thread %d is finished. Return value is %d\n", tid+1, retval);
    return tid;
}

void* phello(void *arg){
    printf("Hello! pthread %lu\n", pthread_self());
    return 0;
}

void uthread_test(int n) {
    uthread_t thread[n];

    for (int i = 0; i < n; ++i) {
        thread[i] = uthread_create(hello, (void*)n);
    }
    int retval;
    uthread_join(thread[0], &retval);
    printf("Return value is %d\n", retval);
}

void pthread_test(int n) {
    pthread_t thread[n];

    for (int i = 0; i < n; ++i) {
        pthread_create(&thread[i], NULL, phello, NULL);
    }

    for (int j = 0; j < n; ++j) {
        pthread_join(thread[j], NULL);
        printf("Return value from thread %lu: %d\n", thread[j], 0);
    }
    pthread_t tmp;
    pthread_create(&tmp, NULL, phello, NULL);
    pthread_join(tmp, NULL);
}

int main(){
    int n;
    printf("Enter the number of threads: ");
    scanf("%d", &n);
    printf("My output:\n");
    uthread_test(n);
    printf("Standard output:\n");
    pthread_test(n);
}