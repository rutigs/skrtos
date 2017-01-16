/* 
 * ctsw.c : context switcher
 *  
 * This file is responsible for the creation of processes
 *
 * Public functions:
 * - void context_init();
 *      This sets the IDT entry for interrupts to be called into the context
 *      switch method.
 *
 * - int contextswitch( struct pcb_t *p )
 *      This method saves the kernel state and stack and switches into the process.
 *      It then receives interrupts, saves the process' state and stack, and returns
 *      to the dispatcher for the interrupt to processed. 
 *
 *      Returns: 
 *      -> register eax of the interrupting process which corresponds to the syscall
 *      it is request
 */

#include <xeroskernel.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

/* _ISREntryPoint prototype declaration for IDT init */
void _ISREntryPoint();

/* static pointers */
static void *k_stack;
static unsigned long *ESP;

/* holder for system call return value */
static int EAX;

/*
 * Initialize the context switcher and IDT entries.
 */
void context_init() {
  set_evec(SYS_CALL_ID, &_ISREntryPoint);
}


/*
 * Context Switching routine.
 * Save's kernel state and stack and switch into process
 * Receive interrupt and return to dispatch for handling
 *
 * Arguments
 * - Pointer to process control block to switch into
 *
 * Returns
 * - eax of interrupting process
 */
int contextswitch( struct pcb_t *p ) {
  ESP = p->esp;
  EAX = p->result_code; 

  __asm __volatile( "         \
    pushf                   \n\
    pusha                   \n\
    movl  %%esp, k_stack    \n\
    movl  ESP, %%esp        \n\
    popa                    \n\
    movl  EAX, %%eax        \n\
    iret                    \n\
  _ISREntryPoint:           \n\
    pusha                   \n\
    movl  %%esp, ESP        \n\
    movl  k_stack, %%esp    \n\
    popa                    \n\
    popf                    \n\
      "
    :
    : 
    : "%eax"
    );

  p->esp = ESP;
  struct context_frame *context = p->esp;
  return context->eax;
}
