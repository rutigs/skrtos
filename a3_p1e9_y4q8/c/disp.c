/* disp.c : dispatcher
 */

#include <xeroskernel.h>
#include <i386.h>
#include <xeroslib.h>
#include <stdarg.h>

static pcb      *ready_head = NULL;
static pcb      *ready_tail = NULL;

extern int      idle_pid;


static int  kill(pcb *currP, int pid);


void     dispatch( void ) {
/********************************/

    pcb         *p;
    int         r;
    funcptr     fp;
    int         stack;
    va_list     ap;
    char        *str;
    int         len;
    int         signal_num;
    int         pid;
    int         fd;
    void       *buf;
    int         buflen;
    unsigned long command;
    int         device_no;
    Bool         block;

    for( p = next(); p; ) {

      // If the process has been signaled setup the stack to handle it
      if ( !p->processing && p->signals > 0 ) {
        setup_sigtramp( p );
        p->processing = 1;
      }

      r = contextswitch( p );
      switch( r ) {

      case( SYS_CREATE ):
        ap = (va_list)p->args;
        fp = (funcptr)(va_arg( ap, int ) );
        stack = va_arg( ap, int );
	      p->ret = create( fp, stack );
        break;

      case( SYS_YIELD ):
        ready( p );
        p = next();
        break;

      case( SYS_STOP ):
        stop(p);
        p = next();
        break;

      case ( SYS_KILL ):
        ap = (va_list)p->args;
	      pid = va_arg( ap, int );
        signal_num = va_arg( ap, int );
        p->ret = signal( pid, signal_num );
        if (p->ret == -1) {
          p->ret = -712;
        } else if (p->ret == -2) {
           p->ret = -651;
        }
        break;

      case ( SYS_KILL_PROC ):
        ap = (va_list)p->args;
        p->ret = kill(p, va_arg( ap, int ) );
        break;

      case (SYS_CPUTIMES):
	      ap = (va_list) p->args;
	      p->ret = getCPUtimes(p, va_arg(ap, processStatuses *));
	      break;

      case( SYS_PUTS ):
    	  ap = (va_list)p->args;
    	  str = va_arg( ap, char * );
    	  kprintf( "%s", str );
    	  p->ret = 0;
    	  break;

      case( SYS_GETPID ):
      	p->ret = p->pid;
      	break;

      case( SYS_SLEEP ):         
      	ap = (va_list)p->args;
      	len = va_arg( ap, int );
      	sleep( p, len );
      	p = next();
      	break;

      case( SYS_TIMER ):
      	tick();
      	//kprintf("T");
      	p->cpuTime++;
      	ready( p );
      	p = next();
      	end_of_intr();
      	break;

      case( SYS_SIGHANDLER ):
        ap = (va_list)p->args;
        signal_num = va_arg( ap, int );
        void (*newhandler)(void *) = va_arg( ap, void* );
        void (**oldhandler)(void *) = va_arg( ap, void** );
        p->ret = sighandler( p, signal_num, newhandler, oldhandler );
        break;

      case( SYS_SIGRETURN ):
        ap = (va_list)p->args;
        int *old_sp = va_arg( ap, int * );
        // Get the return value we pushed onto the stack
        p->ret = *(old_sp - 1);
        p->esp = old_sp;
        context_frame *cf = (context_frame *) p->esp;
        cf->esp = (int)old_sp;
        p->processing = 0;
        break;

      case( SYS_WAIT ):
        ap = (va_list)p->args;
        pid = va_arg( ap, int );
        pcb *target_proc = findPCB(pid);
        if ( !target_proc ) {
          p->ret = -1;
          ready( p );
        } else {
          p->ret = 0;
          enqueue(&target_proc->wait_head, &target_proc->wait_tail, p);
          p->state = STATE_WAIT;
          p->waiting_proc = target_proc;
        }
        p = next();
        break;

      case( SYS_OPEN ):
        ap = (va_list)p->args;
        device_no = va_arg( ap, int );
        block = di_open( p, device_no );
        if (block) p = next();
        break;

      case( SYS_CLOSE ):
        ap = (va_list)p->args;
        fd = va_arg( ap, int );
        block = di_close( p, fd );
        if (block) p = next();
        break;

      case( SYS_WRITE ):
        ap = (va_list)p->args;
        fd = va_arg( ap, int );
        buf = va_arg( ap, void* );
        buflen = va_arg( ap, int );
        block = di_write( p, fd, buf, buflen );
        if (block) p = next();
        break;

      case( SYS_READ ):
        ap = (va_list)p->args;
        fd = va_arg( ap, int );
        buf = va_arg( ap, void* );
        buflen = va_arg( ap, int );
        block = di_read( p, fd, buf, buflen ); 
        if (block) p = next();
        break;

      case( SYS_IOCTL ):
        ap = (va_list)p->args;
        fd =  va_arg( ap, int );
        command = va_arg( ap, unsigned long );
        va_list ioctl_args = va_arg( ap, va_list );
        block = di_ioctl( p, fd, command, ioctl_args );
        if (block) p = next();
        break;

      case( SYS_KEYBD ):
        keyboard_int_handler();
        end_of_intr();
        break;

      default:
        kprintf( "Bad Sys request %d, pid = %d\n", r, p->pid );
      }
    }

    kprintf( "Out of processes: dying\n" );

    for( ;; );
}

extern void dispatchinit( void ) {
/********************************/

  //bzero( proctab, sizeof( pcb ) * MAX_PROC );
  memset(proctab, 0, sizeof( pcb ) * MAX_PROC);
}



extern void     ready( pcb *p ) {
/*******************************/
    enqueue(&ready_head, &ready_tail, p);
    p->state = STATE_READY;
}

extern pcb      *next( void ) {
/*****************************/

  pcb *next_proc = dequeue( &ready_head, &ready_tail );

  if ( next_proc->pid == idle_pid ) {

    pcb *next_next_proc = dequeue( &ready_head, &ready_tail );

    if ( next_next_proc ) {
      // User process exists
      // Ready the idle process again and Return the next_next_proc
      ready( next_proc );
      return next_next_proc;
    } else {
      // Return the idle process because there are still sleeping processes
      return next_proc;
    }
  } else {
    // Baseline return the next proc
    return next_proc;
  }
}

/*
 * Adds node to end of queue.
 *
 * Arguments:
 *  head - pointer to address of head of queue
 *  tail - pointer to address of tail of queue
 *  node - pointer to pcb to add to queue
 */
void enqueue(pcb **head, pcb **tail, pcb *node) {
  node->next = NULL;
  node->prev = *tail;

  if( *tail ) {
    ( *tail )->next = node;
  } else {
    ( *head ) = node;
  }

  ( *tail ) = node;
}

/*
 * Removes and returns the pcb at the front of the queue.
 *
 * Arguments:
 *  head - pointer to address of head of queue
 *  tail - pointer to address of tail of queue
 *
 * Returns:
 *  the pcb at the front of the queue
 *  NULL is queue is empty
 */
pcb *dequeue(pcb **head, pcb **tail) {

  pcb *node = *head;

  if( node ) {
    *head = node->next;
    node->next = NULL;
    if( ! *head ) {
      *tail = NULL;
    } else {
      ( *head )->prev = NULL;
    }
  }

  return node;
}

/*
 * Remove node from queue.
 *
 * Arguments:
 *  head - pointer to address of head of queue
 *  tail - pointer to address of tail of queue
 *  node - pointer to pcb to remove from queue
 */
void remove(pcb **head, pcb **tail, pcb *node) {

  if (node == *head ) {
    dequeue(head, tail);
    return;
  }

  node->prev->next = node->next;

  if (node == *tail) {

    *tail = node->prev;
    ( *tail )->next = NULL;

  } else {
    node->next->prev = node->prev;
  }

  node->next = NULL;
  node->prev = NULL;
}

/*
 * Finds the user process associated with pid
 *
 * Arguments:
 *   - pid of the pcb to find
 *
 * Returns
 *   NULL if process does not exist, or is the idle process,
 *        or has stopped
 *   pcb of the process otherwise
 */
pcb *findPCB( int pid ) {
    int	i;

    // return NULL for idle process
    if (pid == 0) {
      return( NULL );
    }

    for( i = 0; i < MAX_PROC; i++ ) {
        if( proctab[i].pid == pid && proctab[i].state != STATE_STOPPED ) {
            return( &proctab[i] );
        }
    }

    return( NULL );
}


// This function takes a pointer to the pcbtab entry of the currently active process.
// The functions purpose is to remove the process being pointed to from the ready Q
// A similar function exists for the management of the sleep Q. Things should be re-factored to
// eliminate the duplication of code if possible. There are some challenges to that because
// the sleepQ is a delta list and something more than just removing an element in a list
// is being preformed.

void removeFromReady(pcb * p) {

  if (!ready_head) {
    kprintf("Ready queue corrupt, empty when it shouldn't be\n");
    return;
  }

  remove(&ready_head, &ready_tail, p);

  if (ready_head == NULL) { // This should never happen
      kprintf("Kernel bug: Where is the idle process\n");
  }
}

/*
 * Stop process and notify waiting processes.
 *
 * Arguments:
 *  p - pointer to the pcb
 */
void stop(pcb *p) {

  p->state = STATE_STOPPED;

  while( p->wait_head ) {
    pcb *wake = dequeue(&p->wait_head, &p->wait_tail);
    wake->ret = 0;
    ready(wake);
  }
}


// This function is the system side of the sysgetcputimes call.
// It places into a the structure being pointed to information about
// each currently active process.
//  p - a pointer into the pcbtab of the currently active process
//  ps  - a pointer to a processStatuses structure that is
//        filled with information about all the processes currently in the system
//

extern char * maxaddr;

int getCPUtimes(pcb *p, processStatuses *ps) {

  int i, currentSlot;
  currentSlot = -1;

  // Check if address is in the hole
  if (((unsigned long) ps) >= HOLESTART && ((unsigned long) ps <= HOLEEND))
    return -1;

  //Check if address of the data structure is beyone the end of main memory
  if ((((char * ) ps) + sizeof(processStatuses)) > maxaddr)
    return -2;

  // There are probably other address checks that can be done, but this is OK for now


  for (i=0; i < MAX_PROC; i++) {
    if (proctab[i].state != STATE_STOPPED) {
      // fill in the table entry
      currentSlot++;
      ps->pid[currentSlot] = proctab[i].pid;
      ps->status[currentSlot] = p->pid == proctab[i].pid ? STATE_RUNNING: proctab[i].state;
      ps->cpuTime[currentSlot] = proctab[i].cpuTime * MILLISECONDS_TICK;
    }
  }

  return currentSlot;
}

// This function takes 2 paramenters and kills the process with pid:
//  currP  - a pointer into the pcbtab that identifies the currently running process
//  pid    - the proces ID of the process to be killed.
//
static int  kill(pcb *currP, int pid) {
  pcb * targetPCB;
  
  //kprintf("Current pid %d Killing %d\n", currP->pid, pid);
  
  if (pid == currP->pid) {   // Trying to kill self
    return -2;
  }
    
  if (!(targetPCB = findPCB( pid ))) {
    // kprintf("Target pid not found\n");
    return -1;
  }

  
  // PCB has been found,  and the proces is either sleepign or running.
  // based on that information remove the process from 
  // the appropriate queue/list.

  if (targetPCB->state == STATE_SLEEP) {
    // kprintf("Target pid %d sleeping\n", targetPCB->pid);
    removeFromSleep(targetPCB);
  }

  if (targetPCB->state == STATE_READY) {
    // remove from ready queue
    // kprintf("Target pid %d is ready\n", targetPCB->pid);
    removeFromReady(targetPCB);
  }

  if (targetPCB->state == STATE_WAIT) {
    remove(&targetPCB->waiting_proc->wait_head,
           &targetPCB->waiting_proc->wait_tail, targetPCB);
  }

  // Close any open processes.
  int i;
  for( i = 0; i < MAX_PROC_DEVICES; i ++ ) {
    devsw *dev = targetPCB->fd_tab[i];
    if ( (int) dev != NULL_DEVICE) (dev->dev_close)(targetPCB, dev);
  }

  // Check other states and do state specific cleanup before stopping
  // the process 
  // In the new version the process will not be marked as stopped but be 
  // put onto the readyq and a signal marked for delivery. 

  targetPCB->state = STATE_STOPPED;
  return 0;
}


