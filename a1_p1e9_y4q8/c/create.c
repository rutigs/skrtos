/* 
 * create.c : create a process
 * 
 * This file is responsible for the creation of processes
 *
 * Public functions:
 * - void create_init();
 *      This method initializes the process id counter that is used to give processes
 *      a unique pid on creation.
 *
 * - int create( void (*func)(void), int stack );
 *      This method takes the process function pointer and the stack size,
 *      assigns it to a PCB and initializes its slot in the pcb table.
 *      The stack is allocated using kmalloc, the context frame is initialized
 *      and the pid is set. The process is then added to the ready queue.
 *
 *      Returns: 
 *      -> 0 if the arguments for the function or stack are invalid
 *      -> 0 if the process table is full
 *      -> 0 if their is insufficient memory for the stack
 *      -> 1 on success
 *
 * - Bool test_create_process();
 *      Entry point to testing function test_create
 *
 *      Returns:
 *      -> False if any of the tests fail
 *      -> True  iff all of the tests pass
 */

#include <xeroskernel.h>

/* Your code goes here. */
#define SAFETY_MARGIN 8   /* Margin left at end of stack to prevent overwritting mem_header */

/* PCB Table */
extern struct pcb_t pcb_arr[];

/* Counter for keeping track of process ids */
unsigned long pid_count;

/*
 * Initialize process id counter
 */ 
void create_init()
{
  pid_count = 1;
}

/*
 * Create new process and add it to the ready queue.
 *
 * Arguments
 * - function pointer for the process
 * - stack size
 *
 * Return
 * - 1 on success
 * - 0 on failure.
 */
int create( void (*func)(void), int stack ) 
{

  if (func == NULL || stack <= 0 ){
    kprintf("Invalid Arguments to create: func is: 0x%x, stack is: 0x%x\n", func, stack);
    return 0;
  }

  /* Loop through pcb_arr to find a free pcb */
  struct pcb_t *pcb = NULL;
  int i = 0;
  for(i; i < PCB_ARR_SIZE ; i++) {
    if (pcb_arr[i].state == STOPPED) {
      pcb = &pcb_arr[i];
      break;
    }
  }

    /* Error out if no available processes */
  if (pcb == NULL) {
    kprintf("Process limit reached - Out of free PCB.\n");
    return 0;
  }

  /* allocate memory for process */
  void *memory = kmalloc(stack); 
  if (memory == NULL) {
    kprintf("kmalloc failed in create()\n");
    return 0;
  }

  /* initialize process stack */
  struct context_frame *init_context = memory + stack - sizeof(struct context_frame) - SAFETY_MARGIN;
  init_context->esp = init_context;
  init_context->ebp = init_context;
  init_context->iret_eip = func;
  init_context->iret_cs = getCS();
  init_context->eflags = 0;


  /* Initialize the PCB */
  pcb->state = READY;
  pcb->memory = memory;
  pcb->esp = init_context;
  pcb->next_pcb = NULL;
  pcb->result_code = OK;
  pcb->pid = pid_count;

  pid_count++;

  /* Add the new process to the ready queue */
  ready(pcb);

  return 1;
}


/* ========================================================================= */
/*                               Internal Tests                              */
/* ========================================================================= */

/* Test methods declaration */
Bool test_create( void );

/*
 * All the Create Process tests.
 * Declared inside xeroskernel, used to hide internal tests.
 * Return true if all tests pass.
 */
Bool test_create_process() 
{
  kprintf("\nTesting: Running create.c tests. \n");

  Bool result = test_create();

  if (result) {
    kprintf("\nTesting: create.c tests passed. \n");
  }
  return result;
} 


/*
 * Test create()
 *
 * Return true if tests pass.
 */
Bool test_create() 
{
  int result;

  /* Test 1: invalid function pointer */
  result = create(NULL, 12345);

  if ( result != 0 ) {
    kprintf("create test failed at create.c: Test 1: NULL func pointer not caught.\n");
    return FALSE;
  }


  /* Test 2: invalid stack size */
  result = create(&test_create, 0);

  if ( result != 0 ) {
    kprintf("create test failed at create.c: Test 2: invalid stack size not caught.\n");
    return FALSE;
  }


  /* Test 3: PCB table full */
  dispatch_init();

  int i = 0;
  for(i; i < PCB_ARR_SIZE ; i++) {
    pcb_arr[i].state = BLOCKED;
  }

  result = create(&test_create, 1096);

  if ( result != 0 ) {
    kprintf("create test failed at create.c: Test 3: created process when PCB table full.\n");
    return FALSE;
  }


  /* Test 4: Stack size too large */
  kmeminit();
  dispatch_init();
  result = create(&test_create, 999999999999);

  if ( result != 0 ) {
    kprintf("create test failed at create.c: Test 4: out of memory error not thrown.\n");
    return FALSE;
  }


  /* Test 5: create a process */
  kmeminit();
  dispatch_init();
  result = create(&test_create, 1096);

  if (pcb_arr[0].pid != pid_count-1) {
    kprintf("create test failed at create.c: Test 5: PCB pid not correct");
    return FALSE;
  }

  if ( pcb_arr[0].state != READY ) {
    kprintf("create test failed at create.c: Test 5: PCB state not READY\n");
    return FALSE;
  }

  if ( pcb_arr[0].result_code != OK ) {
    kprintf("create test failed at create.c: Test 5: PCB result_code not OK\n");
    return FALSE;
  }

  struct context_frame *init_context = pcb_arr[0].esp;

  if ( init_context != pcb_arr[0].memory + 1096 - sizeof(struct context_frame) - SAFETY_MARGIN) {
    kprintf("create test failed at create.c: Test 5: init_context position incorrect\n");
    return FALSE;
  }

  if ( init_context->esp != init_context ) {
    kprintf("create test failed at create.c: Test 5: esp not on stack\n");
    return FALSE;
  }

  if ( init_context->ebp != init_context ) {
    kprintf("create test failed at create.c: Test 5: ebp not on stack\n");
    return FALSE;
  }

  if ( init_context->iret_eip != &test_create ) {
    kprintf("create test failed at create.c: Test 5: iret_eip incorrect\n");
    return FALSE;
  }

  if ( init_context->eflags != 0 ) {
    kprintf("create test failed at create.c: Test 5: eflags incorrect\n");
    return FALSE;
  }

  return TRUE;
}

