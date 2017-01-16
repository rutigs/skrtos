/* disp.c : dispatch between processes and syscalls
 *
 * This file is responsible for management of processes and their multitasking
 *
 * Public functions:
 * - void dispatchinit();
 *      This method initializes the process table and the head and tail of the ready queue
 *
 * - void dispatch();
 *      This method handles switching and multitasking from interrupts.
 *      It starts by context switching into the first process on the ready queue
 *      and upon interrupts it will switch back into the dispatch to handle the
 *      request. Arguments are checked in the handling functions itself (except for sysputs)
 *      Dispatch will loop until the program is terminated.
 *
 * - void ready( pcb *p );
 *      Adds the given process' control block to the end of the ready queue
 *
 * - pcb *next( void );
 *      Gets the next ready process from the ready queue
 *
 * - int kill_process( int pid, int proc_pid );
 *      Takes the given pid and attempts to terminate that running process depending on whether
 *      it actually exists or if a process is attempting to kill itself.
 *
 * - int cleanup( pcb *p );
 *      Cleans up the resources being used by the process and reset the corresponding fields inside
 *      process table.
 *
 * - void notify_blocked_proc( pcb *p );
 *      When a process dies/is killed we notify blocked processes that may be waiting for it
 *
 * - pcb *get_pcb_by_id( int pid );
 *      Retrieves the pcb with given pid from the process table
 *
 * - int is_running( int pid );
 *      Checks if a process with given pid is running (on the ready queue)
 *
 * - void enqueue(pcb **head, pcb **tail, pcb *node);
 *      Enqueues a process on the given queue
 *
 * - pcb *dequeue(pcb **head, pcb **tail);
 *      Dequeues a process from the given queue
 *
 * - void remove(pcb **head, pcb **tail, pcb *node);
 *      Remove a process from the given queue
 *
 * - void insertAfter(pcb **head, pcb **tail, pcb *prev_node, pcb *node_to_add);
 *      Insert a process after another process in the given queue
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <i386.h>

/* Head and tail for the ready, recv_any, and sleeping queues */
static pcb      *ready_head = NULL;
static pcb      *ready_tail = NULL;
pcb      *recv_any_head = NULL;
pcb      *recv_any_tail = NULL;
pcb      *sleep_head = NULL;
pcb      *sleep_tail = NULL;

extern int      idle_pid;

/* Partially given method
 *
 * Begin dispatching and context switching between processes
 * Handles interrupts from the context switcher and handle syscalls and multitasking
 */
void     dispatch( void ) {
/********************************/

  pcb           *p;
  int           r;
  funcptr       fp;
  int           stack;
  char          *str;
  int           pid;
  va_list       ap;

  for( p = next(); p; ) {

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
      cleanup(p);
      p = next();
      break;
    case( SYS_GETPID ):
      p->ret = p->pid;
      break;
    case( SYS_PUTS ):
      ap = (va_list)p->args;
      str = va_arg(ap, char * );
      if ( str ) {
        kprintf( str );
      }
      break;
    case( SYS_KILL ):
      ap = (va_list)p->args;
      pid = va_arg( ap, int );
      p->ret = kill_process( pid, p->pid );
      break;
    case( SYS_SEND ):
      send( p );
      p = next();
      break;
    case( SYS_RECV ):
      recv( p );
      p = next();
      break;
    case( TIMER_INT_SYS ):
      tick();
      ready( p );
      p = next();
      // Signal an end of interrupt to rearm the hardware
      end_of_intr();
      break;
    case( SYS_SLEEP ):
      sleep(p);
      p = next();
      break;
    default:
      kprintf( "Bad Sys request %d, pid = %d\n", r, p->pid );
    }
  }

  kprintf( "Out of processes: dying\n" );

  for( ;; );
}

/*
 * Partially given implementation
 *
 * Initializes the process table and the ready queue
 */
extern void dispatchinit( void ) {
/********************************/

  memset(proctab, 0, sizeof( pcb ) * MAX_PROC);
  ready_head = NULL;
  ready_tail = NULL;
  recv_any_head = NULL;
  recv_any_tail = NULL;
  sleep_head = NULL;
  sleep_tail = NULL;
}

/*
 * Adds a given process to the ready queue and gives its state ready
 */
extern void     ready( pcb *p ) {
/*******************************/

  p->state = STATE_READY;
  enqueue(&ready_head, &ready_tail, p);
}

/*
* Returns the next process off the ready to queue. Returns the first process
* on the receive any queue if no user processes are on the ready queue.
* Returns the idle process if there are no user processes or blocked receive
* any processes.
*
* Arguments:
*   none
*
* Returns:
*   pointer to the next ready pcb if exists
*   pointer to a blocked recv_any process if there is no processes for it recv from
*   pointer to the idle_proc if no other process is ready
*/
pcb      *next( void ) {
/*****************************/

  pcb *next_proc = dequeue( &ready_head, &ready_tail );

  if ( next_proc->pid == idle_pid ) {

    pcb *next_next_proc = dequeue( &ready_head, &ready_tail );

    if ( next_next_proc ) {
      // User process exists
      // Ready the idle process again and Return the next_next_proc
      ready( next_proc );
      return next_next_proc;
    } else if ( sleep_head ) {
      // Return the idle process because there are still sleeping processes
      return next_proc;
    } else {
      // No user process available (ready or asleep)
      // We need to check if there is a proccess on the recv_any blocked queue
      next_next_proc = dequeue( &recv_any_head, &recv_any_tail );

      if ( next_next_proc ) {
        // if there a process blocked on receive any, return it with error
        next_next_proc->ret = -1;
        next_next_proc->mem_locn = NULL;
        next_next_proc->dest_pid = NULL;
        next_next_proc->buffer = NULL;
         // Ready the idle process again and Return the next_next_proc
        ready( next_proc );
        return next_next_proc;
      } else {
        // Return the idle process
        return next_proc;
      }
    }
  } else {
    // Baseline return the next proc
    return next_proc;
  }
}


/*
 * Handles the process killing for the syskill system call
 *
 * Arguments:
 *  pid - the ID of the process to kill
 *  proc_pid - the ID of the process that initiated the kill request
 *
 * Returns:
 *   0 on success.
 *  -1 if the target process does not exist
 *  -2 if pid is the same as proc_pid
 */
int kill_process( int pid, int proc_pid )
{
  if ( pid == proc_pid ) {
    return -2;
  }

  // kprintf( "%s - killing pid %d\n", __func__, pid );

  pcb *p = get_pcb_by_id(pid);
  pcb *dest;

  if ( !p ) {
    return -1;
  }

  // Remove process from it's queue
  switch( p->state ) {
    case( STATE_READY ):
      remove(&ready_head, &ready_tail, p);
      break;
    case( STATE_BLOCKED_SEND ):
      dest = get_pcb_by_id(p->dest_pid);
      remove(&dest->send_head, &dest->send_tail, p);
      break;
    case( STATE_BLOCKED_RECV ):
      if ( p->dest_pid != 0 ) {
        dest = get_pcb_by_id(p->dest_pid);
        remove(&dest->recv_head, &dest->recv_tail, p);
      } else {
        remove(&recv_any_head, &recv_any_tail, p);
      }
      break;
    case( STATE_BLOCKED_SLEEP ):
      remove(&sleep_head, &sleep_tail, p);
      break;
    default:
      kprintf( "Illegal Process State. pid = %d, state = %d\n", p->pid, p->state);
  }

  cleanup(p);
  return 0;
}

/*
 * Clean up and free resources used by the process.
 * Frees the PCB and allocated memory and notifies blocked processes.
 *
 * Arguments:
 *  p - pointer to the pcb
 *
 * Returns:
 *  0 on success
 *  -1 on error
 */
int cleanup( pcb *p )
{
  int result = kfree(p->mem);
  notify_blocked_proc(p);

  p->state = STATE_STOPPED;
  p->esp = NULL;
  p->ret = NULL;
  p->next = NULL;
  p->mem = NULL;
  p->args = NULL;

  return result;
}

/*
 * In the case that a process dies/ends while another process may be blocked waiting for it this
 * will update the blocked process accordingly
 *
 * Arguments:
 *  p - the pcb of the process that is terminated
 */
void notify_blocked_proc( pcb *p ) {

  // Return all blocked sends
  pcb *blocked_proc = dequeue(&p->send_head, &p->send_tail);
  while (blocked_proc) {
    // kprintf( "%s - Found process waiting for %d\n", __func__, p->pid );
    blocked_proc->ret = -1;
    blocked_proc->mem_locn = NULL;
    blocked_proc->dest_pid = NULL;
    blocked_proc->buffer = NULL;
    ready(blocked_proc);
    blocked_proc = dequeue(&p->send_head, &p->send_tail);
  }

  // Return all blocked recvs
  blocked_proc = dequeue(&p->recv_head, &p->recv_tail);
  while (blocked_proc) {
    // kprintf( "%s - Found process waiting for %d\n", __func__, p->pid );
    blocked_proc->ret = -1;
    blocked_proc->mem_locn = NULL;
    blocked_proc->dest_pid = NULL;
    blocked_proc->buffer = NULL;
    ready(blocked_proc);
    blocked_proc = dequeue(&p->recv_head, &p->recv_tail);
  }
}

/*
 * Gets the pcb with ID pid.
 *
 * Arguments:
 *  pid - the id of the process
 *
 * Returns
 *  the pcb with that id, if exists
 *  NULL if no process with pid exists
 */
pcb *get_pcb_by_id( int pid ) {
  int index = ( pid % MAX_PROC ) - 1;
  if ( proctab[index].state != STATE_STOPPED &&
       proctab[index].pid == pid ) {
    return &proctab[index];
  } else {
    return NULL;
  }
}

/*
 * Returns 0 if the process is not on the ready queue
 * Returns 1 if the process is on the ready queue
 */
int is_running( int pid ) {

  pcb* p = get_pcb_by_id(pid);

  if ( p ) {
    return p->state == STATE_READY;
  }

  return 0;
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
  insertAfter(head, tail, *tail, node);
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
 * Add node on queue after prev_node
 *
 * Arguments:
 *  head - pointer to address of head of queue
 *  tail - pointer to address of tail of queue
 *  prev_node - pointer to pcb afterwhich node_to_add should be
 *              NULL if node_to_add should be added before head 
 *  node_to_add - pointer to pcb to add to queue
 */
void insertAfter(pcb **head, pcb **tail, pcb *prev_node, pcb *node_to_add) {

  if (prev_node == NULL ) {

    if (*head) {
      node_to_add->next = *head;
      node_to_add->prev = NULL;
      ( *head )->prev = node_to_add;
      *head = node_to_add;
    } else {
      *head = node_to_add;
      *tail = node_to_add;
      node_to_add->next = NULL;
      node_to_add->prev = NULL;
    } 
    return;
  }

  node_to_add->prev = prev_node;
  node_to_add->next = prev_node->next;
  prev_node->next = node_to_add;

  if (prev_node == *tail) {

    *tail = node_to_add;

  } else {
    node_to_add->next->prev = node_to_add;
  }
}

