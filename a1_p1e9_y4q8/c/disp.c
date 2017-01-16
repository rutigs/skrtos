/* disp.c : dispatch between processes and syscalls
 * 
 * This file is responsible for management of processes and their multitasking
 *
 * Public functions:
 * - void disp_init();
 *      This method takes the process control block table and initializes each
 *      process' state to stopped, initializes the head and the tail of the ready
 *      process queue.
 *
 * - void dispatch();
 *      This method handles switching and multitasking from processes interrupts.
 *      It starts by context switching into the first process on the ready queue
 *      and upon interrupts it will switch back into the dispatch to handle the 
 *      syscall request. Syscall argument validity is checked in the handling 
 *      functions itself (create() in create.c). Dispatch will loop until the 
 *      program is terminated.
 *
 * - Bool test_dispatcher();
 *      Entry point to testing function test_dispatcher
 *
 *      Returns:
 *      -> False if any of the tests fail
 *      -> True  iff all of the tests pass
 */

#include <xeroskernel.h>

/* Your code goes here */

/* Internal helpers */
struct pcb_t*   next();
void            cleanup( struct pcb_t *process );

/* Global PCB array */
struct pcb_t pcb_arr[PCB_ARR_SIZE];

/* Initialize pointers to the pcb queue for managing getting and adding new processes */
struct pcb_t *ready_pcb_queue;
struct pcb_t *ready_pcb_queue_tail;
struct pcb_t *blocked_pcb_queue;


/* 
 * Initialize PCB table and process queues.
 */
void dispatch_init() {
  int i = 0;
  for(i; i < PCB_ARR_SIZE ; i++) {
    pcb_arr[i].state = STOPPED;
  }

  ready_pcb_queue = NULL;
  ready_pcb_queue_tail = NULL;
  blocked_pcb_queue = NULL;
}


/* 
 * Begin dispatching and context switching between processes
 * Handles interrupts from the context switcher and passes them to their appropriate syscall
 */
void dispatch( )
{ 
  kprintf("\nEntering dispatch \n");

  struct pcb_t *process = next();

  for( ;; ) {

    process->state = RUNNING;

    /* Context switch into process of the ready queue and wait for interrupt request */
    int request = contextswitch( process );  //TODO check syscall arguments??
    
    /* Get system call arguments */
    struct context_frame *context = process->esp; 
    unsigned long *args = context->edx;

    /* Match the request to its syscall and process it */
    switch( request ) {
      case( SYS_CREATE ):
        /* kprintf("SYS_CREATE reached\n"); */

        /* if create() failed, return SYSERR to process */
        /* argument validity are checked in the create() function itself */
        if (create(*args, *(args + 1)) == 0 ) {
          process->result_code = SYSERR;
        } else {
          process->result_code = OK;
        }
        break;
      case( SYS_YIELD ):
        /* kprintf("SYS_YIELD reached\n"); */
        ready( process );
        process->result_code = OK;
        process = next();
        break;
      case( SYS_STOP ):
        /* kprintf("SYS_STOP reached\n"); */
        cleanup( process );
        process = next();
        break;
    }
  }
}

/* 
 * Gets the next ready pcb from the ready queue
 *
 * Returns pointer to the pcb.
 */ 
struct pcb_t* next()
{
  if (ready_pcb_queue == NULL ) {
    return NULL;
  }

  struct pcb_t *next_pcb = ready_pcb_queue;
  
  if (ready_pcb_queue == ready_pcb_queue_tail) {
      ready_pcb_queue_tail = ready_pcb_queue->next_pcb;
  }
  ready_pcb_queue = ready_pcb_queue->next_pcb;

  return next_pcb;
}

/*
 * Adds the pcb to the end of the ready queue
 */
void ready( struct pcb_t *new_pcb )
{
  if (ready_pcb_queue == NULL ) {
    ready_pcb_queue = new_pcb;
  } else {
    ready_pcb_queue_tail->next_pcb = new_pcb;
  }

  ready_pcb_queue_tail = new_pcb;
  ready_pcb_queue_tail->next_pcb = NULL;
  new_pcb->state = READY;
}


/*
 * Clean up and free resources used by the process.
 * Frees the PCB and allocated memory.
 */
void cleanup( struct pcb_t *process )
{
  kfree(process->memory);

  process->state = STOPPED;
  process->pid = 0;
  process->esp = NULL;
  process->result_code = NULL;
  process->next_pcb = NULL;
  process->memory = NULL;
}



/* ========================================================================= */
/*                               Internal Tests                              */
/* ========================================================================= */

/* Test methods declaration */
Bool test_dispatchinit( void );
Bool test_next( void );
Bool test_ready( void );
Bool test_cleanup( void );


/*
 * All the Dispatcher tests.
 *
 * Return true if all tests pass.
 */
Bool test_dispatcher() 
{
  kprintf("\nTesting: Running disp.c tests. \n");
  Bool result = TRUE;

  result &= test_dispatchinit();
  result &= test_ready();
  result &= test_next();
  result &= test_cleanup();

  if (result) {
    kprintf("\nTesting: disp.c tests passed. \n");
  }
  return result;
} 


/*
 * Test dispatchinit()
 *
 * Return true if tests pass.
 */
Bool test_dispatchinit() 
{
  dispatch_init();

  if (ready_pcb_queue != NULL) {
    kprintf("dispatchinit test failed at disp.c: ready_pcb_queue not initialized to NULL.");
    return FALSE;
  }

  if (ready_pcb_queue_tail != NULL) {
    kprintf("dispatchinit test failed at disp.c: ready_pcb_queue_tail not initialized to NULL.");
    return FALSE;
  }

  int i = 0;
  for(i; i < PCB_ARR_SIZE ; i++) {
    if (pcb_arr[i].state != STOPPED) {
      kprintf("dispatchinit test failed at disp.c: pcb_arr[%d].state not initialized to STOPPED", i);
      return FALSE;
    }
  }

  return TRUE;
}

/*
 * Test ready()
 *
 * Return true if tests pass.
 */
Bool test_ready() 
{

  /* Test 1: add one pcb to ready queue */
  dispatch_init();
  ready(pcb_arr);

  if (ready_pcb_queue != pcb_arr) {
    kprintf("ready test failed at disp.c: Test 1: ready_pcb_queue incorrectly updated.");
    return FALSE;
  }

  if (ready_pcb_queue_tail != pcb_arr) {
    kprintf("ready test failed at disp.c: Test 1: ready_pcb_queue_tail incorrectly updated.");
    return FALSE;
  }

  if (ready_pcb_queue->state != READY) {
    kprintf("ready test failed at disp.c: Test 1: ready_pcb_queue state not READY.");
    return FALSE;
  }

  /* Test 2: add two pcb to ready queue */
  dispatch_init();
  ready(pcb_arr);
  ready(&pcb_arr[1]);

  if (ready_pcb_queue != pcb_arr) {
    kprintf("ready test failed at disp.c: Test 2: ready_pcb_queue incorrectly updated.");
    return FALSE;
  }

  if (ready_pcb_queue_tail != &pcb_arr[1]) {
    kprintf("ready test failed at disp.c: Test 2: ready_pcb_queue_tail not updated.");
    return FALSE;
  }


  /* Test 3: add multiple pcb to ready queue */
  dispatch_init();
  ready(pcb_arr);
  ready(&pcb_arr[1]);
  ready(&pcb_arr[3]);
  ready(&pcb_arr[7]);

  if (ready_pcb_queue != pcb_arr) {
    kprintf("ready test failed at disp.c: Test 3: ready_pcb_queue incorrectly updated.");
    return FALSE;
  }

  if (ready_pcb_queue_tail != &pcb_arr[7]) {
    kprintf("ready test failed at disp.c: Test 3: ready_pcb_queue_tail not updated.");
    return FALSE;
  }

  if (ready_pcb_queue->next_pcb != &pcb_arr[1]) {
    kprintf("next test failed at disp.c: Test 3: nodes not linked.");
    return FALSE;
  }

  if (ready_pcb_queue->next_pcb->next_pcb != &pcb_arr[3] 
      || ready_pcb_queue->next_pcb->next_pcb->next_pcb != ready_pcb_queue_tail ) {
    kprintf("next test failed at disp.c: Test 3: ready queue structure incorrect.");
    return FALSE;
  }
  
  return TRUE;
}

/*
 * Test next()
 *
 * Return true if tests pass.
 */
Bool test_next() 
{

  /* Test 1: empty ready queue */
  dispatch_init();

  struct pcb_t *process = next();

  if (process != NULL) {
    kprintf("next test failed at disp.c: Test 1: did not return NULL on empty queue.");
    return FALSE;
  }

  /* Test 2: one element ready queue */
  dispatch_init();
  ready(pcb_arr);

  process = next();

  if (process != pcb_arr) {
    kprintf("next test failed at disp.c: Test 2: result incorrect.");
    return FALSE;
  }

  if (ready_pcb_queue != NULL) {
    kprintf("next test failed at disp.c: Test 2: ready_pcb_queue not updated.");
    return FALSE;
  }

  if (ready_pcb_queue_tail != NULL) {
    kprintf("next test failed at disp.c: Test 2: ready_pcb_queue_tail not updated.");
    return FALSE;
  }


  /* Test 3: three element ready queue */
  dispatch_init();
  ready(pcb_arr);
  ready(&pcb_arr[2]);
  ready(&pcb_arr[5]);

  process = next();

  if (process != pcb_arr) {
    kprintf("next test failed at disp.c: Test 3: result incorrect.");
    return FALSE;
  }

  if (ready_pcb_queue != &pcb_arr[2]) {
    kprintf("next test failed at disp.c: Test 3: ready_pcb_queue not updated.");
    return FALSE;
  }

  return TRUE;
}

/*
 * Test cleanup()
 *
 * Return true if tests pass.
 */
Bool test_cleanup() 
{

  /* Test 1: free resources properly */
  kmeminit();
  dispatch_init();
  create(&test_cleanup, 54321);
  struct pcb_t *process = next();

  if (process->state == STOPPED ) {
    kprintf("cleanup test failed at disp.c: Test 1: test initilization error.");
    return FALSE;
  }

  cleanup(process);

  if (process->pid != NULL ) {
    kprintf("cleanup test failed at disp.c: Test 1: pid not NULL.");
    return FALSE;
  }

  if (process->memory != NULL ) {
    kprintf("cleanup test failed at disp.c: Test 1: memory not NULL.");
    return FALSE;
  }
  
  if (process->state != STOPPED ) {
    kprintf("cleanup test failed at disp.c: Test 1: state not STOPPED.");
    return FALSE;
  }
  
  if (process->next_pcb != NULL ) {
    kprintf("cleanup test failed at disp.c: Test 1: next_pcb not NULL.");
    return FALSE;
  }

  if (process->esp != NULL ) {
    kprintf("cleanup test failed at disp.c: Test 1: esp not NULL.");
    return FALSE;
  }

  if (process->result_code != NULL ) {
    kprintf("cleanup test failed at disp.c: Test 1: result_code not NULL.");
    return FALSE;
  }

  return TRUE;
}



