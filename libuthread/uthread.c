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
int add_main_thread_to_scheduler(){
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
    mainThread->waitingThreadTID = INT16_MAX; //TODO: this might not be a very clever way
    mainThread->status = RUNNING;
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
    newThread->waitingThreadTID = INT16_MAX; //set short max as default value

    newThread->TID = threadScheduler.NEXT_TID;
    //check if it is our first time to call thread_create
    if(newThread->TID == 1) {
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
    newThread->status = READY;

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

    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    //change the status
    nextThread->status = RUNNING;
    currentThread->status = READY;
    //enqueue
    queue_enqueue(threadScheduler.readyThreads, currentThread);
    threadScheduler.runningThread = nextThread;

    preempt_enable();

    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);
}

uthread_t uthread_self(void)
{
    assert(threadScheduler.runningThread);
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
    uthread_t dest = (uthread_t)(long)arg;
    if(ele->TID == dest)
        return 1;
    else
        return 0;
}

/*
 * destroy myQueue
 * if there is still element inside myQueue
 * we also free that element
 */
void destroy_queue(queue_t myQueue){
    TCB *tmp;
    while(queue_length(myQueue) != 0){
        queue_dequeue(myQueue, (void**)&tmp);
        uthread_ctx_destroy_stack(tmp->ctx->uc_stack.ss_sp);
        free(tmp->ctx);
    }
    queue_destroy(myQueue);
}

/*
 * we need to bring that thread back to ready list
 * so that it can collect exit status of current thread
 */
void activate_waiting_thread(uthread_t tid){
    TCB *waitingThread = NULL;
    //we dont change the threadScheduler, so it is fine to be preempted
    queue_iterate(threadScheduler.waitingThreads, find_thread, (void*)tid, (void**)&waitingThread);
    assert(waitingThread);

    preempt_disable();

    queue_delete(threadScheduler.waitingThreads, waitingThread);
    waitingThread->status = READY;
    queue_enqueue(threadScheduler.readyThreads, waitingThread);

    preempt_enable();
}

void exit_program() {
    //we dont want to switch context when we are cleaning up
    preempt_disable();

    destroy_queue(threadScheduler.readyThreads);
    destroy_queue(threadScheduler.waitingThreads);
    destroy_queue(threadScheduler.finishedThreads);
    uthread_ctx_destroy_stack(threadScheduler.runningThread->ctx->uc_stack.ss_sp);
    free(threadScheduler.runningThread->ctx);
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
    if(currentThread->waitingThreadTID != INT16_MAX)
        activate_waiting_thread(currentThread->waitingThreadTID);

    TCB *nextThread = NULL;

    preempt_disable();

    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    assert(nextThread);
    currentThread->status = FINISHED;
    currentThread->retval = retval;
    queue_enqueue(threadScheduler.finishedThreads, currentThread);
    nextThread->status = RUNNING;
    threadScheduler.runningThread = nextThread;

    preempt_enable();

    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);
}

/*
 * when we call this function, we guarantee that
 * reapedThread is in finished list and reapedThread!=NULL
 * we collect the return value and free allocated memory
 * Return value:
 * reapThread->retval
 */
int reap_sthread(TCB *reapedThread) {
    preempt_disable();

    queue_delete(threadScheduler.finishedThreads, reapedThread);
    int retval = reapedThread->retval;
    //free the memory allocated for reapedThread
    uthread_ctx_destroy_stack(reapedThread->ctx->uc_stack.ss_sp);
    free(reapedThread->ctx);

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
    queue_iterate(threadScheduler.finishedThreads, find_thread, (void*)tid, (void**)&threadTID);
    if(threadTID){
        int tmp = reap_sthread(threadTID);
        if(retval)
            *retval = tmp;
        return 0;
    }

    //if @tid is still an active thread
    //we have to bring it to execution and block current thread
    TCB *currentThread = threadScheduler.runningThread;
    queue_iterate(threadScheduler.readyThreads, find_thread, (void*)tid, (void**)&threadTID);
    //fail to find @tid thread in ready list
    if(!threadTID){
        queue_iterate(threadScheduler.waitingThreads, find_thread, (void*)tid, (void**)&threadTID);
        //fail to find @tid in waiting list
        if(!threadTID)
            return -1;
    }

    TCB *nextThread = NULL;

    preempt_disable();

    //we put current thread into the waiting list
    //block it until threadTID finish its execution
    currentThread->status = WAITING;
    threadTID->waitingThreadTID = currentThread->TID;
    queue_enqueue(threadScheduler.waitingThreads, currentThread);

    //bring next readyThread to execute
    queue_dequeue(threadScheduler.readyThreads, (void**)&nextThread);
    assert(nextThread);
    nextThread->status = RUNNING;
    threadScheduler.runningThread = nextThread;

    preempt_enable();

    //switch context to next ready thread
    uthread_ctx_switch(currentThread->ctx, nextThread->ctx);

    //when currentThread is resumed, threadTID should already be finished
    //we collect its exit status and free it
    assert(threadTID->status == FINISHED);
    int tmp = reap_sthread(threadTID);
    if(retval)
        *retval = tmp;
    return 0;
}