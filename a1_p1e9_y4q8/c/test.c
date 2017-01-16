#include <xeroskernel.h>

/*
 * Testing entrypoint for our Kernel
 */

/* Internal helpers declaration */
void test_user_process_master( void );
void test_user_process_child( void );

/* Global var for holding test values */
static int successful_child;

/* 
 * This will be the baseline method called for testing purposes
 * to test specific functionally simply uncomment that test section 
 */ 
void run_unit_tests()
{
  test_mem_manager();
  test_dispatcher();
  test_create_process();

  kprintf("\n------------------------------------\n");
  kprintf("End of Unit Tests. Entering Kernel.");
  kprintf("\n------------------------------------\n");
}

/* 
 * Tests the kernel by creating root process that spawns child 
 * processes until the process limit is hit. 
 */ 
void run_kernel_test() 
{
  kprintf("\nEntering kernel test: Spins up child processes until "
  "the process limit is reached.\n");

  /* Initialize the kernel */
  kmeminit();
  dispatch_init();
  context_init();
  create_init();

  create(&test_user_process_master, 4096);
  dispatch();
}



/*
 * Master process for testing kernel
 */
void test_user_process_master()
{ 
  Bool test_result = TRUE;

  int children = 0;
  int result = OK;
  while (result != SYSERR) {
    result = syscreate(&test_user_process_child, 4096);
    if (result == OK) children++;
  }

  if (children != PCB_ARR_SIZE - 1) {
    kprintf("\nKernel Test failed at test_user_process_master: number of children processes incorrect.\n");
    test_result = FALSE;
  }

  successful_child = 0;

  sysyield();
  sysyield();

  if (successful_child != PCB_ARR_SIZE - 1) {
    kprintf("\nKernel Test failed at test_user_process_master: Children process context switching failed.\n");
    kprintf("successful_child = %d", successful_child);
    test_result = FALSE;
  }

  if (test_result == TRUE ) {
    kprintf("\n\nKernel Test passed.\n");
  }

  kprintf("\n---------------------------------------------\n");
  kprintf("End of Kernel Tests. Check print statements.");
  kprintf("\n----------------------------------------------\n");

  for(;;) ; /* loop forever */
}

/*
 * Child process for testing kernel
 */
void test_user_process_child() 
{

  kprintf("| New Process Child");
  sysyield();
  successful_child++;
  sysstop();
}

