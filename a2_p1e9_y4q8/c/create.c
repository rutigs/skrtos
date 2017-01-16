/*
 * create.c : create a process
 *
 * This file is responsible for the creation of processes
 *
 * Public functions:
 *
 * - int create( funcptr fp, size_t stackSize );
 *      This method takes the process function pointer and the stack size,
 *      assigns it to a PCB and initializes its slot in the pcb table.
 *      The stack is allocated using kmalloc, the context frame is initialized
 *      and the pid is set. The process is then added to the ready queue.
 *
 * - void createinit( void );
 *     this method initializes the pid for all processes to zero
 */

#include <xeroskernel.h>
#include <xeroslib.h>

pcb     proctab[MAX_PROC];

/* make sure interrupts are armed later on in the kernel development  */
#define STARTING_EFLAGS         0x00003200

/*
 * Given method:
 *  Creates a process from a given function pointer and a given stack size.
 */
int create( funcptr fp, size_t stackSize ) {
/***********************************************/

  context_frame       *cf;
  void                *memory;
  pcb                 *p = NULL;
  int                 index;

  // If the stack is too small make it larger
  if( stackSize < PROC_STACK ) {
    stackSize = PROC_STACK;
  }

  for( index = 0; index < MAX_PROC; index++ ) {
    if( proctab[index].state == STATE_STOPPED ) {
      p = &proctab[index];
      break;
    }
  }

  //    Some stuff to help wih debugging
  //    char buf[100];
  //    sprintf(buf, "Slot %d empty\n", index);
  //    kprintf(buf);
  //    kprintf("Slot %d empty\n", index);

  if( !p ) {
    return CREATE_FAILURE;
  }

  memory = kmalloc( stackSize );
  if( !memory ) {
    return CREATE_FAILURE;
  }

  cf = (context_frame *)((unsigned char *)memory + stackSize - 8);
  cf--;

  memset(cf, 0xA5, sizeof( context_frame ));

  cf->iret_cs = getCS();
  cf->iret_eip = (unsigned int)fp;
  cf->eflags = STARTING_EFLAGS;
  // To handle the process trying to "return" or running off its code.
  cf->stackSlots[0] = (unsigned long) *sysstop;

  cf->esp = (int)(cf + 1);
  cf->ebp = cf->esp;
  p->esp = (unsigned long*)cf;
  p->state = STATE_READY;
  p->mem = memory;
  p->send_head = NULL;
  p->send_tail = NULL;
  p->recv_head = NULL;
  p->recv_tail = NULL;


  /* If the pcb has never been used (pid is 0), or next pid is <= 0, restart the sequence.
   * Otherwise, index = ( pid % MAX_PROC ) - 1
   */
  if ( p->pid == 0 || p->pid + MAX_PROC <= 0) {
    p->pid = index + 1;
  } else {
    p->pid += MAX_PROC;
  }

  ready( p );
  return p->pid;
}

/*
 * Initialize the pid of every PCB to 0
 */
void createinit( void ) {

  int index;
  for( index = 0; index < MAX_PROC; index++ ) {
    proctab[index].pid = 0;
  }
}
