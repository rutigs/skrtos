/* 
 * user.c : User processes
 *
 * This file is responsible for code that would take place in user processes that is
 * multitasked by the kernel
 *
 * Public functions:
 * - void root( void );
 *      This method starts the root 'process' in which it uses the syscall create
 *      to create user processes and continues to yield to visualize the interrupting 
 *      and context switching of their code.
 */

#include <xeroskernel.h>

/* Your code goes here */

/* Internal helpers */
void producer (void);
void consumer (void);

/*
 * Root process function
 *
 * Creates the producer and consumer processes for testing
 * context switching and syscalls.
 *
 * First process managed by dispatcher.
 */
void root ( void ) {
  kprintf("Hello World!\n");

  int result;
  result = syscreate(&producer, 4096);\
  if (result == SYSERR ) {
    kprintf("Process Creation failed for producer().\n");
  }

  result = syscreate(&consumer, 4096);
  if (result == SYSERR ) {
    kprintf("Process Creation failed for consumer().\n");
  }

  for(;;) sysyield();
}

/*
 * Producer process function for testing syscalls
 */
void producer( void ) {

  int i;
  for (i = 0; i < 12; i++) {
    kprintf("\n Happy 101st");
    sysyield();
  }
  sysstop();
}

/*
 * Consumer process function for testing syscalls
 */
void consumer ( void ) {
  int i;
  for (i = 0; i < 15; i++) {
    kprintf(" Birthday UBC");
    sysyield();
  }
  sysstop();
}
