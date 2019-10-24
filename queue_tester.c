#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "./libuthread/queue.h"

int find_int(void *data, void *arg){
    int *ele = (int *)data;
    int dest = (int)(long)arg;
    if(*ele == dest)
        return 1;
    else
        return 0;
}

int printer_int(void *data, void *arg){
    if(!data)
        return 1;
    int *tmp = (int *)data;
    printf("Print_int: %d\n", *tmp);
    return 0;
}

void test_dequeue(queue_t myQueue){
    char *str = NULL;
    queue_dequeue(myQueue, (void**)&str);
    assert(str);
    printf("%s\n", str);
    double *tmp = NULL;
    queue_dequeue(myQueue, (void**)&tmp);
    printf("%lf\n", *tmp);
    queue_dequeue(myQueue, (void**)&tmp);
    printf("%lf\n", *tmp);
    int returnVal = queue_iterate(myQueue, printer_int, NULL, NULL);
}

void test_delete(queue_t myQueue){
    int *a = NULL;
    queue_iterate(myQueue, find_int, (void*)2, (void**)&a);
    assert(a != NULL);
    int returnVal = queue_delete(myQueue, a);
    assert(returnVal == 0);
    queue_iterate(myQueue, printer_int, NULL, NULL);
    queue_iterate(myQueue, find_int, (void*)100, (void**)&a);
    assert(a != NULL);
    returnVal = queue_delete(myQueue, a);
    assert(returnVal == 0);
    queue_iterate(myQueue, printer_int, NULL, NULL);
}

void test_iterate(queue_t myQueue){
    int returnVal = queue_iterate(myQueue, printer_int, NULL, NULL);
    assert(returnVal != -1);
    int *a = NULL;
    returnVal = queue_iterate(myQueue, find_int, (void*)1, (void**)&a);
    assert(a != NULL);
    *a = 100;
    int *b = NULL;
    returnVal = queue_iterate(myQueue, find_int, (void*)1, (void**)&b);
    assert(b == NULL);
    returnVal = queue_iterate(myQueue, printer_int, NULL, NULL);
}

void test_enqueue(queue_t myQueue){
    char *str = malloc(50* sizeof(char));
    str = "RNG NMSL WSND";
    queue_enqueue(myQueue, str);
    int *a = malloc(sizeof(int));
    *a = 1;
    queue_enqueue(myQueue, a);
    int *b = malloc(sizeof(int));
    *b = 2;
    queue_enqueue(myQueue, b);
    double c = 3.0;
    queue_enqueue(myQueue, &b);
    double *d = malloc(sizeof(double));
    *d = 4.0;
    queue_enqueue(myQueue, d);
}

int main() {
    printf("This is the unit test file for queue.h\n");

    //create queue
    queue_t myQueue = queue_create();
    assert(myQueue != NULL);

    //enqueue test
    test_enqueue(myQueue);
    assert(queue_length(myQueue) == 5);

    //test iterate
    test_iterate(myQueue);

    //test delete
    test_delete(myQueue);
    assert(queue_length(myQueue) == 3);

    //test dequeue
    test_dequeue(myQueue);
    assert(queue_length(myQueue) == 0);

    //test destroy
    queue_destroy(myQueue);
}