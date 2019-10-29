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
    + destroy queue, if null return -1, else free memory and return 0
  + *queue_enqueue*: 
    + create a new node and link it to the header if queue is not empty.
  + *queue_delete*:
    + Go through every element, find corresponding element, and free it.
  + *queue_iterate*:
    + iterate through every element of queue, apply function on every element.
    + Return: -1 if @func are NULL or not found in queue, 0 otherwise.
  + *queue_length*: get queue length by *queue->numOfElement*.

## Phase 2

### Data Structure Design 
  + To define and control thread, we use two data structure and a enumeration.
  + The enumeration __uthread_s__ enumerates 4 different status of a thread.
  
  + The data structure __uthread_control_block__ is TCB.
    + It is used to store all the information needed, includes five members:
    + __uthread_t TID__: store the ID of thread as unsigned short integer.
    + __uthread_s status__: store one of the four status.
    + __uthread_ctx_t *ctx__: store the current context.
      + uthread_ctx_t is a User-level thread context type
      + This type is an opaque data structure type that 
      + contains a thread's execution context.
      + initialized for the first time when creating a thread with O.
    + __int retval__: store the return value of a thread.
    + __uthread_t waitingThreadTID__: store the waiting thread TID.
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
    + __queue_t readyThreads__: schedule and store the thread in ready state.
    + __queue_t waitingThreads__: schedule and store the thread waiting.
    + __queue_t finishedThreads__: schedule and store the thread in ready state.
    + __queue_t readyThreads__: schedule and store the thread in ready state.
    + __queue_t readyThreads__: schedule and store the thread in ready state.
    ```C
    typedef struct scheduler{
        queue_t readyThreads;
        TCB *runningThread;
        queue_t waitingThreads;
        queue_t finishedThreads;
        uthread_t NEXT_TID;
    }scheduler;
    ```
  





