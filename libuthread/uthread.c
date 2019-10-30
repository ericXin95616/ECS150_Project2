#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <zconf.h>

#include "context.h"
#include "preempt.h"
#include "queue.h"
#include "uthread.h"

/*
 * TCB is used to store all the information
 * we need to know about a thread
 */
typedef struct uthread_control_block{
    uthread_t TID;
    uthread_ctx_t *ctx; //include stack, sigmask, uc_mcontext
    int retval;
    bool isJoined; //indicate whether it is joined by other thread
    uthread_t waitingThreadTID;
}TCB;

/*
 * scheduler is used to coordinate the behaviors
 * of different threads
 */
typedef struct scheduler{
    queue_t readyThreads;
    TCB *runningThread;
    queue_t waitingThreads;
    queue_t finishedThreads;
    uthread_t NEXT_TID;
}scheduler;

scheduler threadScheduler = {NULL, NULL, NULL, NULL, 1};

/*
 * we need to add main thread to threadScheduler
 * Before doing that, we need to create queue for
 * all four queues in threadScheduler
 * Return value:
 * -1 if malloc fail, 0 if success
 * Note!!!
 * For main thread, we do not need to initialize
 * ctx variable! Because whenever the context is
 * switch, context for main thread will be saved
 * automatically by syscall "swapcontext"!(And
 * that's the reason why professor don't initialize
 * ctx[0])
 */
int add_main_thread_to_scheduler()
{
    threadScheduler.readyThreads = queue_create();
    threadScheduler.finishedThreads = queue_create();
    threadScheduler.waitingThreads = queue_create();

    TCB *mainThread = malloc(sizeof(TCB));
    if(!mainThread){
        perror("malloc");
        return -1;
    }
    mainThread->TID = 0;
    mainThread->retval = -1;
    mainThread->isJoined = false; //it should be always false, main will not be joined by other
    mainThread->waitingThreadTID = 0;
    //I think we need to first malloc memory for ctx variable!
    //Do we need to clear this memory?
    mainThread->ctx = malloc(sizeof(uthread_ctx_t));
    threadScheduler.runningThread = mainThread;
    return 0;
}

/*
 *  if it is the first time we call
 *  uthread_create in the main, we need to
 *  register main as a thread in threadScheduler
 */
int uthread_create(uthread_func_t func, void *arg)
{
    TCB *newThread = malloc(sizeof(TCB));
    if(!newThread){
        perror("malloc");
        return -1;
    }
    newThread->retval = -1; //set minus 1 as its initial value
    newThread->isJoined = false;
    newThread->waitingThreadTID = 0;

    newThread->TID = threadScheduler.NEXT_TID;
    //check if TID overflow
    if(newThread->TID == 0)
        return -1;

    //check if it is our first time to call uthread_create
    if(newThread->TID == 1){
        add_main_thread_to_scheduler();
        preempt_start();
    }

    //I think we need to first malloc memory for ctx variable!
    //Do we need to clear this memory?
    uthread_ctx_t *ctx = malloc(sizeof(uthread_ctx_t));
    void *sp = uthread_ctx_alloc_stack();
    if(!sp){
        perror("malloc");
        return -1;
    }

    if(uthread_ctx_init(ctx, sp, func, arg) == -1){
        printf("Fail to initialize context for thread %d\n", newThread->TID);
        return -1;
    }
    newThread->ctx = ctx;

    //disable preempt when change threadScheduler
    preempt_disable();

    queue_enqueue(threadScheduler.readyThreads, newThread);
    ++(threadScheduler.NEXT_TID);

    preempt_enable();

    return newThread->TID;
}

/*
 * we dequeue runningThread and get the current thread
 * we then dequeue readyThread and get the next thread
 * call uthread_ctx_switch to switch context
 */
void uthread_yield(void)
{
    int returnVal = queue_length(threadScheduler.readyThreads);
    //there is no thread that is ready to be execute, thread will continue running;
    if(returnVal == 0)
        return;
    //else, we switch to next thread
    TCB *currentThread = threadScheduler.runningThread;
    TCB *nextThread = NULL;

    preempt_disable();
    //put currentThread in ready status and nextThread in running status
    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    queue_enqueue(threadScheduler.readyThreads, currentThread);
    threadScheduler.runningThread = nextThread;
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);

    preempt_enable();
}

uthread_t uthread_self(void)
{
    return threadScheduler.runningThread->TID;
}

/*
 * find data->TID == arg
 */
int find_thread(void *data, void *arg)
{
    if(!data)
        return 0;

    TCB *ele = (TCB *)data;
    uthread_t *dest = (uthread_t *)arg;
    if(ele->TID == *dest)
        return 1;
    return 0;
}

/*
 * destroy myQueue
 * if there is still element inside myQueue
 * we also free that element
 */
void destroy_queue(queue_t myQueue)
{
    TCB *tmp;
    while(queue_length(myQueue) != 0){
        queue_dequeue(myQueue, (void**)&tmp);
        uthread_ctx_destroy_stack(tmp->ctx->uc_stack.ss_sp);
        free(tmp->ctx);
        free(tmp);
    }
    queue_destroy(myQueue);
}

/*
 * we need to bring that thread back to ready list
 * so that it can collect exit status of current thread
 */
void activate_waiting_thread(uthread_t tid)
{
    TCB *waitingThread = NULL;

    preempt_disable();

    queue_iterate(threadScheduler.waitingThreads, find_thread, (void*)&tid, (void**)&waitingThread);
    queue_delete(threadScheduler.waitingThreads, waitingThread);
    queue_enqueue(threadScheduler.readyThreads, waitingThread);

    preempt_enable();
}

void exit_program()
{
    //we dont want to switch context when we are cleaning up
    preempt_disable();

    destroy_queue(threadScheduler.readyThreads);
    destroy_queue(threadScheduler.waitingThreads);
    destroy_queue(threadScheduler.finishedThreads);
    uthread_ctx_destroy_stack(threadScheduler.runningThread->ctx->uc_stack.ss_sp);
    free(threadScheduler.runningThread->ctx);
    free(threadScheduler.runningThread);
    exit(EXIT_SUCCESS);
}

/*
 * exit is very similiar to yield
 * except that for exit, we put currentThread back
 * into the finished list, because it is finished.
 * And assign retval to thread->retval
 */
void uthread_exit(int retval)
{
    TCB *currentThread = threadScheduler.runningThread;
    //if current thread is main
    //we free everything and quit
    if(currentThread->TID == 0)
        exit_program();

    //if there is a thread waiting current thread
    if(currentThread->isJoined)
        activate_waiting_thread(currentThread->waitingThreadTID);

    TCB *nextThread = NULL;

    preempt_disable();

    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    currentThread->retval = retval;
    queue_enqueue(threadScheduler.finishedThreads, currentThread);
    threadScheduler.runningThread = nextThread;
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);

    preempt_enable();
}

/*
 * when we call this function, we guarantee that
 * reapedThread is in finished list and reapedThread!=NULL
 * we collect the return value and free allocated memory
 * Return value:
 * reapThread->retval
 */
int reap_sthread(TCB *reapedThread)
{
    preempt_disable();

    queue_delete(threadScheduler.finishedThreads, reapedThread);
    int retval = reapedThread->retval;
    //free the memory allocated for reapedThread
    uthread_ctx_destroy_stack(reapedThread->ctx->uc_stack.ss_sp);
    free(reapedThread->ctx);
    free(reapedThread);

    preempt_enable();
    return retval;
}

/*
 * we first move the current thread from running list
 * and we search the next thread inside ready list
 * and we put next thread into waiting list and switch
 * context.
 */
int uthread_join(uthread_t tid, int *retval)
{
    if(tid == 0 || tid == uthread_self() || tid >= threadScheduler.NEXT_TID)
        return -1;
    //we first need to check if @tid is finished
    //if it is, we can directly collect it finished status and return
    TCB *threadTID = NULL;
    TCB *currentThread = threadScheduler.runningThread;

    preempt_disable();
    queue_iterate(threadScheduler.finishedThreads, find_thread, (void*)&tid, (void**)&threadTID);
    preempt_enable();

    if(threadTID){
        //thread already be joined
        if(threadTID->isJoined)
            return -1;
        //thread is not be joined
        preempt_disable();
        threadTID->isJoined = true;
        threadTID->waitingThreadTID = currentThread->TID;
        preempt_enable();

        int tmp = reap_sthread(threadTID);
        if(retval)
            *retval = tmp;
        return 0;
    }

    //if @tid is still an active thread
    //current thread should be blocked and yield
    preempt_disable();
    queue_iterate(threadScheduler.readyThreads, find_thread, (void*)&tid, (void**)&threadTID);
    preempt_enable();
    //fail to find @tid thread in ready list
    if(!threadTID){
        preempt_disable();
        queue_iterate(threadScheduler.waitingThreads, find_thread, (void*)&tid, (void**)&threadTID);
        preempt_enable();
        //fail to find @tid in waiting list
        if(!threadTID)
            return -1;
    }

    //find @tid
    if(threadTID->isJoined)
        return -1;
    TCB *nextThread = NULL;

    preempt_disable();

    //@tid is not joined be other thread, we need to mark it
    threadTID->isJoined = true;

    //we put current thread into the waiting list
    //block it until threadTID finish its execution
    threadTID->waitingThreadTID = currentThread->TID;
    queue_enqueue(threadScheduler.waitingThreads, currentThread);

    //bring next readyThread to execute
    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    threadScheduler.runningThread = nextThread;
    //switch context to next ready thread
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);

    preempt_enable();

    //when currentThread is resumed, threadTID should already be finished
    //we collect its exit status and free it
    int tmp = reap_sthread(threadTID);
    if(retval)
        *retval = tmp;
    return 0;
}