//
// Created by bochao on 10/27/19.
//
#include <stdlib.h>
#include <stdio.h>
#include "uthread.h"

int isPreemptWorking(void *arg) {
    printf("Preemption is working!\nIf it is not, you cannot see me!\n");
    exit(EXIT_SUCCESS);
}

int main() {
    uthread_create(isPreemptWorking, NULL);
    while(1);
}