#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>

#include "context.h"
#include "preempt.h"
#include "queue.h"
#include "uthread.h"

/*
 * uthread_s == uthread_status
 * used to show different status of different threads
 */
typedef enum uthread_s {
    READY,
    RUNNING,
    WAITING,
    FINISHED
}uthread_s;

/*
 * TCB is used to store all the information
 * we need to know about a thread
 */
typedef struct uthread_control_block{
    uthread_t TID;
    uthread_s status;
    uthread_ctx_t *ctx; //include stack, sigmask, uc_mcontext
    int retval;
    uthread_t waitingThreadTID;
}TCB;

/*
 * scheduler is used to coordinate the behaviors
 * of different threads
 */
typedef struct scheduler{
    queue_t readyThreads;
    queue_t runningThreads;//Only one running thread at one time
    queue_t waitingThreads;
    queue_t finishedThreads;
    uthread_t NEXT_TID;
}scheduler;

scheduler threadScheduler = {NULL, NULL, NULL, NULL, 1};

/*
 * add new thread to threadScheduler
 * we need to decide which list this newThread
 * will be added to
 *
 * return value:
 * -1 if newThread is NULL
 * 0 to indicate success
 */
int add_thread_to_scheduler(TCB *newThread) {
   if(!newThread)
       return -1;

   newThread->retval = -1; //set minus 1 as its initial value
   newThread->waitingThreadTID = -1; //set minus 1 as default value
   queue_t *destQueue = NULL;
   switch (newThread->status){
       case READY:
           destQueue = &(threadScheduler.readyThreads);
           break;
       case RUNNING:
           destQueue = &(threadScheduler.runningThreads);
           break;
       case WAITING:
           destQueue = &(threadScheduler.waitingThreads);
           break;
       case FINISHED:
           destQueue = &(threadScheduler.finishedThreads);
           break;
       default:
           break;
   }
   assert(destQueue);
   if(!(*destQueue))
       *destQueue = queue_create();
   assert(*destQueue);
   queue_enqueue(*destQueue, newThread);
   return 0;
}

/*
 * we need to add main thread to threadScheduler
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
int add_main_thread_to_scheduler(){
    TCB *mainThread = malloc(sizeof(TCB));
    if(!mainThread){
        perror("malloc");
        return -1;
    }
    mainThread->TID = 0;
    mainThread->status = RUNNING;
    //I think we need to first malloc memory for ctx variable!
    //Do we need to clear this memory?
    mainThread->ctx = malloc(sizeof(uthread_ctx_t));
    if(add_thread_to_scheduler(mainThread) == -1){
        printf("Fail to add mainThread to threadScheduler\n");
        return -1;
    }
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

    newThread->TID = threadScheduler.NEXT_TID;
    //check if it is our first time to call thread_create
    if(newThread->TID == 1)
        add_main_thread_to_scheduler();

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
    newThread->status = READY;
    if(add_thread_to_scheduler(newThread) == -1) {
        printf("Fail to add thread %d to threadScheduler\n", newThread->TID);
        return -1;
    }
    ++(threadScheduler.NEXT_TID);
    return newThread->TID;
}

/*
 * we dequeue runningThread and get the current thread
 * we then dequeue readyThread and get the next thread
 * call uthread_ctx_switch to switch context
 */
void uthread_yield(void)
{
    TCB *currentThread = NULL;
    assert(queue_length(threadScheduler.readyThreads) > 0);
    queue_dequeue(threadScheduler.runningThreads, (void**)&currentThread);
    TCB *nextThread = NULL;
    int returnVal = queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    //there is no thread that is ready to be execute, process fail to quit
    if(returnVal == -1)
        exit(EXIT_SUCCESS);
    currentThread->status = READY;
    nextThread->status = RUNNING;
    queue_enqueue(threadScheduler.runningThreads, nextThread);
    queue_enqueue(threadScheduler.readyThreads, currentThread);
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);
}

uthread_t uthread_self(void)
{
    TCB *currentThread = NULL;
    queue_dequeue(threadScheduler.runningThreads, (void**)&currentThread);
    assert(currentThread);
    uthread_t TID = currentThread->TID;
    queue_enqueue(threadScheduler.runningThreads, (void*)currentThread);
    return TID;
}

/*
 * find data->TID == arg
 */
int find_thread(void *data, void *arg)
{
    if(!data || !arg)
        return 0;

    TCB *ele = (TCB *)data;
    uthread_t dest = (uthread_t)(long)arg;
    if(ele->TID == dest)
        return 1;
    else
        return 0;
}

/*
 * exit is very similiar to yield
 * except that for exit, we put currentThread back
 * into the finished list, because it is finished.
 * And assign retval to thread->retval
 */
void uthread_exit(int retval)
{
    TCB *currentThread = NULL;
    //assert(queue_length(threadScheduler.readyThreads) > 0);
    queue_dequeue(threadScheduler.runningThreads, (void**)&currentThread);
    TCB *nextThread = NULL;
    int returnVal = queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    //there is no thread that is ready to be execute, process fail to quit
    if(returnVal == -1)
        exit(EXIT_SUCCESS);

    currentThread->status = FINISHED;
    currentThread->retval = retval;
    nextThread->status = RUNNING;
    queue_enqueue(threadScheduler.runningThreads, nextThread);
    queue_enqueue(threadScheduler.finishedThreads, currentThread);

    //if there is a thread waiting current thread
    //we need to bring that thread back to ready list
    //so that it can collect exit status of current thread
    if(currentThread->waitingThreadTID != -1){
        TCB *waitingThread = NULL;
        queue_iterate(threadScheduler.waitingThreads, find_thread,
                (void*)currentThread->waitingThreadTID, (void**)&waitingThread);
        assert(waitingThread);
        queue_delete(threadScheduler.waitingThreads, waitingThread);
        waitingThread->status = READY;
        queue_enqueue(threadScheduler.readyThreads, waitingThread);
    }
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);
}



/*
 * when we call this function, we guarantee that
 * reapedThread is in finished list and reapedThread!=NULL
 * Return value:
 * reapThread->retval
 */
int reap_sthread(TCB *reapedThread) {
    queue_delete(threadScheduler.finishedThreads, reapedThread);
    int retval = reapedThread->retval;

    //free the memory allocated for reapedThread
    uthread_ctx_destroy_stack(reapedThread->ctx->uc_stack.ss_sp);
    free(reapedThread->ctx);
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
    TCB *nextThread = NULL;
    queue_iterate(threadScheduler.finishedThreads, find_thread, (void*)tid, (void**)&nextThread);
    if(nextThread){
        int tmp = reap_sthread(nextThread);
        if(retval)
            *retval = tmp;
        return 0;
    }

    //if @tid is still an active thread
    //we have to bring it to execution and block current thread
    //TODO: what if @tid is in waiting list? when @tid is also blocked?
    TCB *currentThread = NULL;
    queue_dequeue(threadScheduler.runningThreads, (void**)&currentThread);
    queue_iterate(threadScheduler.readyThreads, find_thread, (void*)tid, (void**)&nextThread);
    //fail to find target thread
    if(!nextThread)
        return -1;
    //delete target thread from the ready list
    queue_delete(threadScheduler.readyThreads, nextThread);

    //we put current thread into the waiting list
    //block it until nextThread finish its execution
    currentThread->status = WAITING;
    nextThread->status = RUNNING;
    nextThread->waitingThreadTID = currentThread->TID;
    queue_enqueue(threadScheduler.waitingThreads, (currentThread));
    queue_enqueue(threadScheduler.runningThreads, nextThread);
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);

    //when currentThread is resumed, nextThread should already be finished
    //we collect its exit status and free it
    assert(nextThread->status == FINISHED);
    int tmp = reap_sthread(nextThread);
    if(retval)
        *retval = tmp;
    return 0;
}

