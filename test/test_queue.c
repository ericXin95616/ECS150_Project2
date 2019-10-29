#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <queue.h>
#include <time.h>

#define NORMAL_TEST_ELEMENT_NUM 10
#define COMPLEX_TEST_ELEMENT_NUM 1000000

/*
 * this test we want to test what will happen
 * if we enqueue an element multiple times and delete it
 */
void test_delete_complex()
{
    queue_t new = queue_create();
    int tmp = 0;
    assert(!queue_enqueue(new, (void*)&tmp));
    assert(!queue_enqueue(new, (void*)&tmp));
    assert(!queue_delete(new, (void*)&tmp));
    assert(queue_length(new) == 1);
    //we dequeue new and should get int *tmp
    //we modify dequeue element, check if tmp changes too
    int *a;
    assert(!queue_dequeue(new, (void**)&a));
    *a = 100;
    assert(tmp == 100);
    assert(!queue_destroy(new));
    printf("Complex delete test: success.\n");
}


typedef struct Student_ID_Card{
    char firstChar;
    int id;
}sid;

int initialize_sid(void *data, void *arg)
{
    sid *student = (sid *)data;
    sid *breakpoint = (sid *)arg;
    if(student == breakpoint)
        return 1;
    student->id = 1;
    student->firstChar = 'a';
    return 0;
}

/*
 * For this test, we enqueue user-defined data structure
 * and call a more complex function to modify it.
 * Then, we dequeue it, see if the result is what we expect
 */
void test_iterate_complex()
{
    queue_t new = queue_create();
    sid *students = malloc(COMPLEX_TEST_ELEMENT_NUM * sizeof(sid));
    for (int i = 0; i < COMPLEX_TEST_ELEMENT_NUM; ++i) {
        students[i].firstChar = '\0';
        students[i].id = 0;
        assert(!queue_enqueue(new, (void*)&students[i]));
    }

    /*
     * we use iterate to modify firstChar and id
     * but we only modify [0, breakpoint) data.
     * The other half we will not modify them
     */
    int breakpoint = rand() % COMPLEX_TEST_ELEMENT_NUM;
    assert(!queue_iterate(new, initialize_sid, (void*)&students[breakpoint], NULL));
    sid *tmp;
    for (int j = 0; j < COMPLEX_TEST_ELEMENT_NUM; ++j) {
        assert(!queue_dequeue(new, (void*)&tmp));
        if(j < breakpoint){
            assert(tmp->id == 1);
            assert(tmp->firstChar == 'a');
            continue;
        }
        assert(tmp->id == 0);
        assert(tmp->firstChar == '\0');
    }
    free(students);
    assert(!queue_destroy(new));
    printf("Complex queue iterate test: success.\n");
}

/*
 * For this test, we enqueue large size of data
 * dequeue them. Enqueue them again and dequeue them,
 * see if anything goes wrong
 */
void test_enqueue_dequeue_complex()
{
    queue_t new = queue_create();
    int *data = malloc(COMPLEX_TEST_ELEMENT_NUM * sizeof(int));
    for (int i = 0; i < COMPLEX_TEST_ELEMENT_NUM/2; ++i) {
        assert(!queue_enqueue(new, (void*)&data[i]));
    }

    void *tmp;
    for (int j = 0; j < COMPLEX_TEST_ELEMENT_NUM/2; ++j) {
        assert(!queue_dequeue(new, &tmp));
        assert(tmp == &data[j]);
    }

    for (int i = COMPLEX_TEST_ELEMENT_NUM/2 + 1; i < COMPLEX_TEST_ELEMENT_NUM; ++i) {
        assert(!queue_enqueue(new, (void*)&data[i]));
    }


    for (int j = COMPLEX_TEST_ELEMENT_NUM/2 + 1; j < COMPLEX_TEST_ELEMENT_NUM; ++j) {
        assert(!queue_dequeue(new, &tmp));
        assert(tmp == &data[j]);
    }
    free(data);
    assert(!queue_destroy(new));
    printf("Complex queue enqueue and dequeue test: success.\n");
}

/*
 * This test is to test some special cases
 * or corner cases
 */
void complex_test()
{
    test_enqueue_dequeue_complex();

    test_iterate_complex();

    test_delete_complex();
}

/*
 *  queue is NULL
 */
void test_length_error()
{
    queue_t new = NULL;
    assert(queue_length(new) == -1);
    printf("Error queue length test: success.\n");
}

/*
 * Two cases:
 * 1, queue == NULL & func != NULL
 * 2, queue != NULL & func == NULL
 */
int do_nothing(void *data, void *arg)
{
    return 0;
}

void test_iterate_error()
{
    queue_t new = NULL;
    assert(queue_iterate(new, do_nothing, NULL, NULL) == -1);

    new = queue_create();
    assert(queue_iterate(new, NULL, NULL, NULL) == -1);
    printf("Error queue iterate test: success.\n");
}

/*
 * Three cases:
 * 1, queue == NULL & data != NULL
 * 2, queue != NULL & data == NULL
 * 3, delete a data that does not exist in queue
 */
void test_delete_error()
{
    //first case
    queue_t new = NULL;
    int a = 0;
    assert(queue_delete(new, (void*)&a) == -1);

    //second case
    new = queue_create();
    assert(!queue_enqueue(new, (void*)&a));
    assert(queue_delete(new, NULL) == -1);

    //third case
    int b = 0;
    assert(queue_delete(new, (void*)&b) == -1);
    assert(queue_delete(new, (void*)&a) == 0);
    printf("Error queue delete test: success.\n");
}

/*
 * Three cases:
 * 1, queue == NULL & data != NULL
 * 2, queue != NULL & data == NULL
 * 3, queue is empty & data != NULL
 */
void test_dequeue_error()
{
    //first case
    queue_t new = NULL;
    char *tmp = malloc(sizeof(char));
    assert(tmp && queue_dequeue(new, (void**)&tmp) == -1);

    //second case
    new = queue_create();
    assert(queue_dequeue(new, NULL) == -1);

    //third case
    assert(queue_dequeue(new, (void**)&tmp) == -1);
    queue_destroy(new);

    printf("Error queue dequeue test: success.\n");
}

/*
 * Three cases:
 * 1, queue == NULL & data != NULL
 * 2, queue == NULL & data == NULL
 * 3, queue != NULL & data == NULL
 */
void test_enqueue_error()
{
    //first case
    queue_t new = NULL;
    float tmp;
    assert(queue_enqueue(new, (void*)&tmp) == -1);

    //second case
    assert(queue_enqueue(new, NULL) == -1);

    //third case
    new = queue_create();
    assert(queue_enqueue(new, NULL) == -1);
    printf("Error queue enqueue test: success.\n");
}

/*
 * Two error cases:
 * 1, destroy a NULL queue
 * 2, destroy an non-empty queue
 */
void test_destroy_error()
{
    //destroy a NULL queue
    queue_t new = NULL;
    assert(queue_destroy(new) == -1);

    //destroy an non-empty queue
    int tmp;
    new = queue_create();
    queue_enqueue(new, (void*)&tmp);
    //length of queue should be greater than 0
    //call queue_destroy on an non-empty queue should return -1
    assert(queue_length(new) && queue_destroy(new) == -1);
    int *a = NULL;
    queue_dequeue(new, (void**)&a);
    //new should be empty now
    assert(!queue_length(new));
    queue_destroy(new);
    printf("Error queue destroy test: success.\n");
}

/*
 * this test is testing what happen if we
 * deliberately use API of queue in a wrong way.
 *
 * For example:
 * queue_t is NULL
 * iterate function is not specify
 * call destroy on an non-empty queue
 */
void error_test()
{
    test_destroy_error();

    test_enqueue_error();

    test_dequeue_error();

    test_delete_error();

    test_iterate_error();

    test_length_error();
}

/*
 * helper function for test_iterate
 * find the element whose data equals arg
 */
int normal_find_int(void *data, void *arg)
{
    int *_data = (int*)data;
    int *dest = (int *)arg;
    if(*_data == *dest)
        return 1;
    return 0;
}

/*
 * enqueue @NORMAL_TEST_ELEMENT_NUM element
 * We randomly select an element, use iterate
 * to find that element and delete it.
 * everytime we delete it, we check if length
 * is correct or not
 */
void test_iterate()
{
    queue_t new = queue_create();
    int *data = malloc(NORMAL_TEST_ELEMENT_NUM * sizeof(int));
    for (int i = 0; i < NORMAL_TEST_ELEMENT_NUM; ++i) {
        data[i] = i;
        queue_enqueue(new, (void*)&data[i]);
    }

    int *tmp;
    int iter_retval, del_retval, count = 0;
    //iterate and delete in random order
    while(queue_length(new) != 0) {
        int random = rand() % NORMAL_TEST_ELEMENT_NUM;
        iter_retval = queue_iterate(new, normal_find_int, (void*)&random, (void**)&tmp);
        assert(!iter_retval);
        del_retval = queue_delete(new, tmp);
        if(!del_retval) {
            ++count;
            assert(queue_length(new) == NORMAL_TEST_ELEMENT_NUM - count);
        }
    }
    assert(count == NORMAL_TEST_ELEMENT_NUM);
    free(data);
    assert(!queue_destroy(new));
    printf("Normal queue iterate test: success.\n");
}

/*
 * enqueue @NORMAL_TEST_ELEMENT_NUM element
 * and we delete them in random order
 * see if queue will fail
 */
void test_delete()
{
    queue_t new = queue_create();
    //we have used int, what about double this time?
    double *data = malloc(NORMAL_TEST_ELEMENT_NUM * sizeof(double));
    for (int i = 0; i < NORMAL_TEST_ELEMENT_NUM; ++i) {
        queue_enqueue(new, (void*)&data[i]);
    }

    int retval;
    int count = 0;
    //delete in random order
    while(queue_length(new) != 0){
        int random = rand()%NORMAL_TEST_ELEMENT_NUM;
        retval = queue_delete(new, (void*)&data[random]);
        //found element
        if(!retval) {
            ++count;
            assert(queue_length(new) == NORMAL_TEST_ELEMENT_NUM - count);
        }
    }
    assert(count == NORMAL_TEST_ELEMENT_NUM);
    free(data);
    queue_destroy(new);
    printf("Normal queue delete test: success.\n");
}

/*
 * enqueue @NORMAL_TEST_ELEMENT_NUM element
 * and dequeue them. Everytime we perform enqueue
 * and dequeue operation, we will check whether
 * queue_length return value is correct
 */
void test_length()
{
    queue_t new = queue_create();
    assert(!queue_length(new));
    for (int i = 0; i < NORMAL_TEST_ELEMENT_NUM; ++i) {
        //we don't really care about what we enqueue in this test
        queue_enqueue(new, (void*)&i);
        assert(queue_length(new) == i + 1);
    }
    int *tmp;
    for (int j = 0; j < NORMAL_TEST_ELEMENT_NUM; ++j) {
        queue_dequeue(new, (void**)&tmp);
        assert(queue_length(new) == NORMAL_TEST_ELEMENT_NUM - j - 1);
    }
    assert(!queue_length(new));
    queue_destroy(new);
    printf("Normal queue length test: success.\n");
}

/*
 * enqueue @NORMAL_TEST_ELEMENT_NUM element
 * and dequeue them, check if they have the same address
 */
void test_enqueue_dequeue()
{
    queue_t new = queue_create();
    int *data = malloc(NORMAL_TEST_ELEMENT_NUM * sizeof(int));
    int retval;
    for (int i = 0; i < NORMAL_TEST_ELEMENT_NUM; ++i) {
        retval = queue_enqueue(new, (void*)&data[i]);
        assert(!retval);
    }

    int *tmp;
    for (int j = 0; j < NORMAL_TEST_ELEMENT_NUM; ++j) {
        retval = queue_dequeue(new, (void**)&tmp);
        assert(!retval);
        /*
         * we want to check if they have same address
         * because we enqueue the address, not data
         */
        assert(tmp == data + j);
    }
    free(data);
    queue_destroy(new);
    printf("Normal queue enqueue and dequeue test: success.\n");
}

/*
 * simply create and destroy a queue
 */
void test_create_destroy()
{
    queue_t new = queue_create();
    assert(new);
    int retval = queue_destroy(new);
    assert(!retval);
    printf("Normal queue create and destroy test: success.\n");
}

void normal_test()
{
    //create queue and destroy queue
    test_create_destroy();

    //enqueue and dequeue
    test_enqueue_dequeue();

    //queue_length
    test_length();

    //queue_delete
    test_delete();

    //queue_iterate
    test_iterate();
}

int main() {
    srand(time(0));

    normal_test();

    error_test();

    complex_test();
}