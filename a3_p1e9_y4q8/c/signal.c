/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <i386.h>

/* Your code goes here */

extern char *maxaddr;

/*
 * Sends a signal No. to a specified process
 *
 * Arguments:
 *   - pid of the process we want to signal
 *   - signal number for that process
 *
 * Returns
 *   -1 if the process is invalid
 *   -2 if the signal is invalid
 *    0 otherwise
 */
int signal(int pid, int sig_no) {
  pcb *proc = findPCB( pid );

  // pid is invalid
  if ( !proc ) {
    return -1;
  } 

  // signal is invalid
  if ( sig_no < 0 || sig_no >= MAX_SIGNALS ) {
    return -2;
  }

  // If signal has no hander, ignore signal
  // According to https://piazza.com/class/isp6lqkqfq32kx?cid=314
  if ( proc->sig_handlers[sig_no] == NULL ) return 0;


  if ( proc->state == STATE_SLEEP ) {
    //remove from sleeping queue
    int ticks_left = removeFromSleep(proc);
    proc->state = STATE_READY;
    proc->ret = ticks_left * MILLISECONDS_TICK;
    ready( proc );
  }

  if ( proc->state == STATE_WAIT ) {
    //Remove from waiting queue
    remove(&proc->waiting_proc->wait_head, &proc->waiting_proc->wait_tail, proc);
    proc->state = STATE_READY;
    proc->ret = -2; 
    ready(proc);
  }
 
  if ( proc->state == STATE_READ ) {  
    //Remove from blocked read
    unblock_proc();
    if (proc->ret <= 0 ) proc->ret = -362;
  }

  // set the bit for this signal
  proc->signals |= 1 << sig_no;
  return 0;
}

/*
 * Registers a signal handler for the given process
 *
 * Arguments:
 *   Signal number you want to register the handler for
 *   Function pointer to the new signal handler
 *   Pointer to a variable that points to the old handler.
 *
 * Returns
 *   -1 if signal is invalid
 *   -2 if the handler resides at an invalid address
 *    0 on success
 */
int sighandler(pcb *proc, int signal, void (*newHandler)(void *), void (** oldHandler)(void *)) {

  // signal number is invalid
  if ( signal < 0 || signal > MAX_SIGNALS ) {
    return -1;
  }

  // memory locations for handlers are invalid
  if ( newHandler < 0 || ((char *) newHandler) > maxaddr ) {
    return -2;
  }
  if ( ((unsigned long) newHandler > HOLESTART) && ((unsigned long) newHandler < HOLEEND) ) {
    return -2;
  }

  *oldHandler = proc->sig_handlers[signal];
  proc->sig_handlers[signal] = newHandler;

  return 0;
}

/*
 * Calls the signal handler and returns to the old context
 *
 * Arguments:
 *   Handler for that signal
 *   Context for that handler
 */
void sigtramp(void (* handler)(void *), void *cntx) {
  // Call the signal handler
  handler(cntx);

  // return to the old stack pointer
  syssigreturn(cntx);
}

/*
 * Setup the process stack for sigtramp and sigtramp context
 *
 * Argument:
 *  Process to be setup to handle a signal
 */
void setup_sigtramp(pcb *proc) {

  // Figure out the signal we want to handle
  int signal;
  for ( signal = MAX_SIGNALS - 1; signal >= 0; signal-- ) {
    // if the bit for signal is set, we've found the highest priority signal
    if ( (proc->signals >> signal) & 1 ) {
      // clear the signal because we are handling it
      proc->signals &= ~(1 << signal);
      break;
    }
  }

  // Get that signal handler
  void *handler = proc->sig_handlers[signal];


  // If there is no handler for this signal we can ignore the signal
  if ( !handler ) return;

  // Save the old sp
  int *old_sp = (int *) proc->esp;

  // setup stack
  unsigned int *sp = (unsigned int *) old_sp;

  // Push old return code to save it
  sp--;
  *sp = proc->ret;

  // Push old sp to save it
  sp--;
  *sp = (int)old_sp;

  // Push handler param for sigtramp
  sp--;
  *sp = (int)handler;

  // Push return addr for sigtramp
  sp--;
  *sp = 0;

  // Create the sigtramp context in process space
  struct context_frame *sigtramp_ctx ;
  sp -= sizeof(context_frame) / sizeof(context_frame *);
  sigtramp_ctx = (context_frame*) sp;
  sigtramp_ctx->edi = 0;
  sigtramp_ctx->esi = 0;
  sigtramp_ctx->ebp = (int)sp;
  sigtramp_ctx->esp = (int)sp;
  sigtramp_ctx->ebx = 0;
  sigtramp_ctx->edx = 0;
  sigtramp_ctx->ecx = 0;
  sigtramp_ctx->eax = 0;
  sigtramp_ctx->iret_eip = (int)sigtramp;
  sigtramp_ctx->iret_cs = getCS();
  sigtramp_ctx->eflags = STARTING_EFLAGS | ARM_INTERRUPTS;

  proc->esp = sp;
}
