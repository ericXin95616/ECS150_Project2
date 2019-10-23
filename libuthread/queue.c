#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "queue.h"

/*
 * I want to implement this queue as a
 * linked list. header indicates the
 * "oldest" element. tail indicates the
 * "youngest" element. next indicates
 * next element.
 */
struct node {
    void *data;
    struct node *next;
};

struct queue {
	struct node *header;
    struct node *tail;
	int numOfElement; // convenient to keep track the size of queue
};

/*
 * Create a queue and initialize all the parameters
 */
queue_t queue_create(void)
{
    queue_t myQueue = malloc(sizeof(struct queue));
    if(!myQueue){
        perror("malloc");
        return NULL;
    }
	myQueue->header = NULL;
	myQueue->tail = NULL;
	myQueue->numOfElement = 0;
	return myQueue;
}

/*
 * check to see if it is NULL or not empty
 * if it is, return -1;
 * else release all relevant memory
 */
int queue_destroy(queue_t queue)
{
    if(!queue || queue->numOfElement > 0)
        return -1;
    free(queue);
    return 0;
}

/*
 * if queue is empty, we do not need to malloc
 * new memory. if it is not, we create a new node
 * and link it to the header.
 *
 * Note:enqueue only change the value of tail
 */
int queue_enqueue(queue_t queue, void *data)
{
    if(!queue || !data)
        return -1;

    struct node *newNode = malloc(sizeof(struct node));
    if(!newNode) {
        perror("malloc");
        return -1;
    }
    newNode->data = data;
    newNode->next = NULL;
    //data is the first element
	if(queue->numOfElement == 0) {
	    queue->header = newNode;
	    queue->tail = newNode;
	    ++(queue->numOfElement);
	    return 0;
	}
	//data is not the first element
    queue->tail->next = newNode;
	queue->tail = newNode;
	++(queue->numOfElement);
	return 0;
}

/*
 * check if queue is NULL or void **data is NULL
 * or queue is empty;
 * if it is, return -1;
 * else we give header->data to void**data and
 * free element pointed by header.
 * If there is only one element left, we also
 * need to set tail back to NULL
 */
int queue_dequeue(queue_t queue, void **data)
{
	if(!queue || !data || queue->numOfElement == 0)
	    return -1;

	*data = queue->header->data;
	struct node *tmp = queue->header;
	queue->header = queue->header->next;
	free(tmp);
	--(queue->numOfElement);
	if(queue->numOfElement == 0)
	    queue->tail = NULL;
	return 0;
}

/*
 * Go through every element,
 * find corresponding element,
 * and free it
 */
int queue_delete(queue_t queue, void *data)
{
    if(!queue || !data)
        return -1;
    //if what we find is header
    if(queue->header->data == data){
        queue_dequeue(queue, &data);
        return 0;
    }
    for (struct node *it = queue->header; it->next != NULL ; it = it->next) {
        if(it->next->data != data)
            continue;
        //it->next->data == data
        struct node *tmp = it->next;
        it->next = it->next->next;
        //if we want to free tail
        //we need to set tail to its prev node
        if(tmp == queue->tail)
            queue->tail = it;
        free(tmp);
        --(queue->numOfElement);
        return 0;
    }
    return -1;
}

/*
 * Return: -1 if @queue or @func are NULL, 0 otherwise.
 */
int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
	if(!queue || !func)
	    return -1;
    for (struct node *it = queue->header; it != NULL ; it = it->next) {
        int returnVal = (*func)(it->data, arg);
        if(returnVal == 0)
            continue;
        //returnVal == 1
        if(data != NULL)
            *data = it->data;
        return 0;
    }
    return 0;
}

int queue_length(queue_t queue)
{
	return queue->numOfElement;
}

