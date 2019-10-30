# ECS150 Project 2 Report
  + To briefly organize and be informative, we report data structure first.
  + For each Phase, we will report by functions or lines of code.

## Phase 1

### Data Structure Design 
  + We implement two data structure, __queue__ and __node__.
  + The data structure __queue__ is a linked list to store a thread as a node.
  + A __queue__ struct includes three members:
    + __node *header__: the header node of the linked list.
    + __node *tail__: the tail node of the linked list.
    + __int numOfElement__: store the number of element (i.e. size) of the queue
    ```C
    struct queue {
        struct node *header;
        struct node *tail;
        int numOfElement; // convenient to keep track the size of queue
    };
    ```
  + The __node__ struct is a node of linked list to store a thread.
  + A __node__ struct includes two members:
    + __void *data__: store the data in any type.
    + __struct node *next__: point to the next node in the queue.
    ```C
    struct node {
        void *data;
        struct node *next;
    };
    ```
  + Queue is FIFO, so we design that *header* indicates the oldest element. 
  + *tail* indicates the newest element. *next* indicates the next element.

### Functions
  + We will introduce the function in the logistic order. 
  + *queue_create*:
    + Create a queue and initialize all the parameters
  + *queue_destroy*:
    + destroy queue, if queue is null or not empty return -1.
    + else free memory and return 0.
  + *queue_enqueue*: 
    + create a new node and link it to the header if queue is not empty.
  + *queue_delete*:
    + Go through every element, find corresponding element, and delete it.
  + *queue_iterate*:
    + iterate through every element of queue, apply function on every element.
    + Return: -1 if @queue or @func are NULL, 0 otherwise.
  + *queue_length*: get queue length by *queue->numOfElement*.
  + Return: -1 if @queue is NULL. Length of @queue otherwise.

### Tester
  + *normal_test* includes all tests that user uses queue correctly.
    + *test_create_destroy*: create a queue and then destroy it.
    + *test_enqueue_dequeue*: enqueue 10 elements, then dequeue them all.
    + *test_length*: while enqueuing 10 elements and then the dequeuing them,
      + check length of the queue after every operation.
    + *test_delete*: enqueue 10 element, then delete them in random order.s
    + *test_iterate*: enqueue 10 element, and then iterate all of them.
  + *error_test* includes all tests that user uses queue incorrectly.
    + *test_destroy_error*: destroy a null queue and destroy a nonempty queue.
    + *test_enqueue_error*: enqueue when queue is null, and when data is null.
    + *test_dequeue_error*: dequeue when queue is null, and when data is null.
    + *test_delete_error*: delete when queue or data is null,
      + and when data does not exist in queue.
    + *test_iterate_error*: iterate when queue is null and when function is null
    + *test_length_error*: call queue_length when queue is null.
  + *complex_test* includes all tests that user uses queue in special case.
    + *test_enqueue_dequeue_complex*: 
      + enqueue large number of data and dequeue them all.
    + *test_iterate_complex*:
      + iterate half number of element, and apply function to those element only
      + enqueue user defined/customalized data structure.
    + *test_delete_complex*: enqueue the same data twice and delete once.

## Phase 2

### Data Structure Design 
  + To define and control thread, we use two data structure and a enumeration.
  + The data structure __uthread_control_block__ is TCB.
    + It is used to store all the information needed, includes five members:
    + __uthread_t TID__: store the ID of thread as unsigned short integer.
    + __uthread_s status__: store one of the four status.
    + __uthread_ctx_t *ctx__: store the current context.
      + uthread_ctx_t is a User-level thread context type
      + initialized for the first time when creating a thread with O.
    + __int retval__: store the return value of a thread.
    + __uthread_t waitingThreadTID__: 
      + store the TID of the thread waiting for the current running thread.
    ```C
    typedef struct uthread_control_block{
        uthread_t TID;
        uthread_s status;
        uthread_ctx_t *ctx; //include stack, sigmask, uc_mcontext
        int retval;
        uthread_t waitingThreadTID;
    }TCB;
    ```
  + The data structure __scheduler__ is to coordinate thread behaviors.
  + A __scheduler__ struct includes five members:
    + __uthread_t NEXT_TID__: store the TID of next thread.
    + __queue_t waitingThreads__: schedule and store the blocked thread.
    + __queue_t finishedThreads__: schedule and store the thread in zombie state
    + __queue_t readyThreads__: schedule and store the thread in ready state.
    + __TCB *runningThread__: store the thread in running state as TCB.
    ```C
    typedef struct scheduler{
        queue_t readyThreads;
        TCB *runningThread;
        queue_t waitingThreads;
        queue_t finishedThreads;
        uthread_t NEXT_TID;
    }scheduler;
    ```

### Global Variable
  + *threadScheduler*:
    + catagory threads in 4 different states and schedule/manipulate them.

### Function
  + *add_main_thread_to_scheduler*:
    + add the first thread(main) to the *threadScheduler*.
    + Return value: -1 if malloc fail, 0 if success.
  + *uthread_create*: creates a new thread running the function @func to which 
    + argument @arg is passed, and returns the TID of this new thread.
  + *uthread_yield*: 
    + be called from the currently active and running thread to finish its 
    + execution. collect the return value @retval from a joining thread.
  + *uthread_self*: get thread identifier
    + Return: The TID of the currently running thread.
  + *find_thread*: find a specific thread by TID.
  + *uthread_exit*:
    + Exit from currently running thread.
    + if exit thread is main, called *exit_program*:
      + deallocate all the memory and exit the program.
  + *reap_sthread*: 
    + collect the return value of the sub-thread and deallocated its memory.
    + when we call this function, we guarantee that  
      + reapedThread is in finished list and reapedThread!=NULL.
    + Return value: *reapThread->retval*.

### Tester Files
  + *uthread_hello.c*: 
    + tests the creation of a single thread and its successful return.
  + *uthread_yield.c*:
    + Tests the creation of multiples threads and the fact that a parent thread
      + should get returned to before its child is executed.

## Phase 3

### Function
  + *uthread_join*:
    + makes the calling thread wait for the thread @tid to complete and assign 
      + the return value of the finished thread to @retval (if not NULL).
    + first move the current thread from running list 
    + and search the next thread inside ready list
    + and put next thread into waiting list and switch context.
  
### Tester Files
  + add two test includes yield and join function.
  + *uthread_hello_join.c*:
    + tests the creation of a two thread *hello1* and *hello22*
    + *hello1* calls join function to *hello2*, and test its successful return.
  + *uthread_yield_join.c*:
    + Test: create, yield, and join of multiple thread in complex case.
    + *main* thread* create and join*thread 1*
    + *thread 1* create and join *thread 2*
    + *thread 2* create and join *thread 3*, and yield before and after print.
    + *thread 3* create *thread 4*, yield before print.
    + *thread 4* first yield, then print. 
    + The expected program output: thread 3, then 4, then 2, and then 1.

## Phase 4

### Macro Constant
  + *ELAPSED_TIME*: set to 10000 because 1/(10000 microsecond) = 100 Hertz.

### Function
  + *preempt_start*:
    + initialize the preempt functionality by set signal handler and timer.
      + we need to block every signal except SIGVTALRM, by following code:
      ```C
      sigfillset(&new_action.sa_mask); // mask/prevent all kind of signal
	    sigdelset(&new_action.sa_mask, SIGVTALRM); // mask signal except SIGVTALRM
	    new_action.sa_flags = 0;
      ```
  + *preempt_enable*:
    + enable preempt functionality by reinstall the signal handler.
      + set signal handler to *VTALRM_handler*, which handle VTALRM like yield.
  + *preempt_disable*:
    + enable preempt functionality by uninstall the signal handler.
      + set signal handler to *SIG_IGN*, which is signal ignore.
  + *VTALRM_handler*:
    + handle the timer interrupt by the handler that simulate *uthread_yield*.