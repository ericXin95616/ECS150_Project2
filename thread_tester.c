//
// Created by bochao on 10/22/19.
//
#include "./libuthread/uthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

int hello(void *arg){
    printf("Hello! I am thread %d\n", uthread_self());
    return 0;
}

void* phello(void *arg){
    printf("Hello! pthread %lu\n", pthread_self());
    return 0;
}

void uthread_test(int n) {
    uthread_t thread[n];

    for (int i = 0; i < n; ++i) {
        thread[i] = uthread_create(hello, NULL);
    }

    for (int j = 0; j < n; ++j) {
        int retval;
        uthread_join(thread[j], &retval);
        printf("Return value from thread %d: %d\n", thread[j], retval);
    }

    uthread_join(uthread_create(hello, NULL), NULL);
    printf("a\n");
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